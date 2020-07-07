#include "process.h"
#include "syscall.h" // for execute and halt functions
#include "../filesystem.h"
#include "../lib.h"
#include "../paging.h"
#include "../x86_desc.h"
#include "../driver/terminal.h"

/* 0 defaults to only switching tasks on terminal chnages, 1 enables schedular code from PIT ints*/
int sched_enable = 1;

pcb_t* processes[NUM_PROCESSES];
uint8_t process_exit_code;

/**
 * Initializes the PCB memory locations.
 * INPUT: None
 * OUTPUT: None
 * EFFECT:
 *   - Compute the PCB offset addresses for each PCB.
 *   - Store each into processes table.
 *   - Initialize each PCB to its initial state.
 */
void init_pcb () {
    uint8_t fd, pid;
    // STEP 1: Initialize ghostd process (0) to NULL
    processes[0] = NULL;
    // STEP 2: Iterate thru pids 1 to 8
    for (pid = 1; pid < NUM_PROCESSES; pid++) {
        /* Base address of PCB: 8MB - 8kb*pid */
        processes[pid] = (pcb_t*) (KERNEL_AREA_BOTTOM - (pid * PROCESS_KERNEL_STACK_SIZE));
        processes[pid]->in_use = 0;
        processes[pid]->pid = pid;
        processes[pid]->parent_pid = NULL;
        processes[pid]->args[0]=0; // Clear the string
        for (fd = 0; fd < NUM_FILE_DESCRIPTORS; fd++) {
            processes[pid]->file_descriptors[fd].flags = 0;
        }
    }
}

/**
 * Executes the given file. This should be called by the syscall wrapper.
 * INPUT: command (string): The command
 * OUTPUT: None
 * RETURN: Status code from the process
 * EFFECT:
 * - Parse args
 * - Check file validity
 * - Set up paging
 * - Load file into memory
 * - Create PCB / Open FDs
 * - Prepare for context switching
 * - Push IRET context to stack
 * - IRET
 * - Wait for halt() to transfer control back, then quit
 */
int32_t execute(const str command) {
    int32_t result;  // Stores last command's result
    uint8_t tmpbuffer[PROCESS_HEADER_BLOCK_LENGTH];  // Used for checking magic and loading EIP
    uint8_t pid = 0;  // PID to be used for the process
    void* entry_eip;
    char args[SIZE_INPUT_BUFFER];
    char filename[SIZE_INPUT_BUFFER];

    // TODO: integrate get args
    /* STEP 1: Parse the args */
    result = get_args_from_cmd(command, filename, args);
    // printf("BRIEFING===\nEXE=%s\nPARAM=%s\n", filename, args);
    if (result == -1) {
        printf("Failed to parse command: %d. (step1)\n", result);
        goto bail;
    }
    dentry_t exec_dentry;
    // strncpy((char*)filename, "shell", 6);
    /* STEP 2: Check file validity */
    result = read_dentry_by_name(filename, &exec_dentry);
    if (result == -1) {
        printf("Executable file does not exist: %d. (step2)\n", result);
        goto bail;
    }
    // Check the magic string
    result = file_read(exec_dentry.inode_num, tmpbuffer, sizeof(tmpbuffer), 0);
    if (result == -1) {
        printf("Error reading magic number from file: %d. (step2)\n", result);
        goto bail;
    }
    if (*((uint32_t*) tmpbuffer) != EXECUTABLE_MAGIC) {
        printf("File is not an executable, magic = %x\n", tmpbuffer);
        goto bail;
    }

    /* STEP 3: Find a PID and confirm the entrypoint */
    entry_eip = (void*) *((uint32_t*) (tmpbuffer + PROCESS_EIP_LOCATION));
    pid = 1;
    for (pid = 1; pid < NUM_PROCESSES; pid++) {
        if (!processes[pid]->in_use) break;
    }
    if (pid >= NUM_PROCESSES) {
        printf("Can't allocate a PID for the process.\n");
        goto bail;
    }

    /* STEP 4: Set up paging */
    set_page_for_process(pid);

    /* STEP 5: Load file into memory */
    load_program(filename);

    /* STEP 6: Create PCB, Open stdin and stdout */
    processes[pid]->in_use = 1;
    processes[pid]->parent_pid = terminals[active_terminal].pid;
    processes[pid]->esp0 = (tss.esp0 = KERNEL_AREA_BOTTOM - pid*PROCESS_KERNEL_STACK_SIZE - 4);
    tss.ss0 = KERNEL_DS;
    strncpy((int8_t*)processes[pid]->args, (int8_t*)args, SIZE_INPUT_BUFFER);

    /* Switch to this PID as the current process */
    terminals[active_terminal].pid = pid;
    /* Open STDIN/OUT */
    processes[pid]->file_descriptors[FD_STDIN].flags = 1;
    processes[pid]->file_descriptors[FD_STDIN].operations_table = DRIVER_TERMINAL;
    processes[pid]->file_descriptors[FD_STDOUT].flags = 1;
    processes[pid]->file_descriptors[FD_STDOUT].operations_table = DRIVER_TERMINAL;
    // TODO: Put the operations table index in these two FDs.

    /* STEP 7: Prepare for context switching */
    // Save the Parent ESP and EBP for returning control
    asm volatile (
        "movl %%esp, %0;"
        "movl %%ebp, %1;"
        :"=r"(processes[pid]->parent_esp), "=r"(processes[pid]->parent_ebp)
    );

    /* STEP 8: Push IRET context to stack; IRET */
    // TODO: Step 8: Complete the ASM
    asm volatile (
        //"cli;"
        /*changing data segment values*/
        "movw $0x002B, %%ax;"  // USER_DS
        "movw %%ax, %%ds;"
        /* next three instructions might not be needed, these segment registers are not used(?) added just in case*/
        "movw %%ax, %%es;"
        "movw %%ax, %%fs;"
        "movw %%ax, %%gs;"
        "pushl $0x002B;"      // push user segment USER_DS
        "pushl  $0x083FFFFC;"  // push user stack pointer = 132MB-4B PROCESS_ESP_LOCATION

        "pushfl;"           // pushing the eflags
        /* modify the IF bit in the flags to make sure interrupts are enabled once user mode is entered*/
        "orl $0x200, (%%eax);"

        "pushl $0x0023;"  // Pushes user code segement USER_CS
        "pushl %0;"  // Push EIP for code
        "iret;"
        "halt_handoff_point:;" // For halt to return here
        :
        : "r"(entry_eip)
        : "%eax"
    );

    /* STEP 9: Return point from halt() */
    return process_exit_code;

    // Standard exception throwing format, courtesy of Riverbed Inc
    bail:
    printf("Execution failed.\n");
    return -1;
}

/**
 * Halts the current process.
 * INPUT: status (byte): Exit code
 * OUTPUT: None
 * RETURN: None
 * EFFECT:
 * - Restore parent data
 * - Restore parent paging
 * - Close any relevant FDs
 * - Jump to execute()'s return
 */
int32_t halt(uint8_t status) {
    int i;
    process_exit_code = status;
    pcb_t* cur_pcb = processes[terminals[active_terminal].pid];
    uint8_t parent_pid = processes[terminals[active_terminal].pid]->parent_pid;

    // STEP 1: Clear the PCB for this process
    processes[terminals[active_terminal].pid]->in_use = 0;
    processes[terminals[active_terminal].pid]->parent_pid = NULL;

    // STEP 2: Close all files
    for (i = 0; i < NUM_FILE_DESCRIPTORS; i++) {
        if (cur_pcb->file_descriptors[i].flags) close(i);
    }
    // STEP 3: Set current pid to parent
    terminals[active_terminal].pid = parent_pid;
    if (parent_pid) {
        // STEP 4: Set page to parent
        set_page_for_process(parent_pid);
        // STEP 5: Restore TSS, esp, ebp to parent
        tss.esp0 = processes[parent_pid]->esp0;
        asm volatile (
                "movl %1, %%ebp;"
                "movl %0, %%esp;"
                "jmp halt_handoff_point;"
                :
                : "r"(cur_pcb->parent_esp), "r"(cur_pcb->parent_ebp)
                : "%eax"
            );
    } else {
        terminals[active_terminal].pid = parent_pid;
        execute("shell");
    }
    return 0; // only for warning suppression
}

/* int32_t vidmap(uint8_t **screen_start)
 * Inputs:  ** screen_start double pointer 
 * Return Value: screen_start 

 * Function: This function is just used to call vidmap_user_page which does most of the work. 
   Return vidmap_user_page(screen_start).  */
int32_t vidmap(uint8_t **screen_start) {

    return vidmap_user_page(screen_start); // just used as helper
}

/* void switch_to_process()
 * assumes that pid currently exectuting is in terminals[previous_terminal].pid, and that
 * terminals[active_terminal].pid contains the pid that needs to be switched to.  
 */
void switch_to_process() {
    /* Save the current ESP and EBP in the previous processes PCB for switching back to it */
    asm volatile (
        "movl %%esp, %0;"
        "movl %%ebp, %1;"
        :"=g"(processes[terminals[previous_terminal].pid]->context_esp), "=g"(processes[terminals[previous_terminal].pid]->context_ebp)
    );
    /* save current process tss info */
    processes[terminals[previous_terminal].pid]->context_esp0 = tss.esp0;

    /* if there isnt a process running in the terminal that's being switched to, launch one and return*/
    if (processes[terminals[active_terminal].pid] == 0) {execute("shell"); return;}

    /* update tss for incoming process*/
    tss.esp0 = processes[terminals[active_terminal].pid]->context_esp0;

    /* set up paging appropriately for the process being switched to*/
    set_page_for_process(terminals[active_terminal].pid);

    /* load the stack registers appropriately for the return to the new process  */
        asm volatile (
        "movl %0, %%esp;"
        "movl %1, %%ebp;"
        :
        :"g"(processes[terminals[active_terminal].pid]->context_esp), "g"(processes[terminals[active_terminal].pid]->context_ebp)
    );
    return;
}
