/**
 * Process related data structures and functions.
 * PCB, execute, halt, etc.
 */
#pragma once

#include "../lib.h"
#include "../driver/terminal.h"

/* We have 9 processes, 0 is the ghostd process that doesn't exist.
 * Just kidding. Reserwe 0 (NULL) for "no process"
 */
#define NUM_PROCESSES 7
#define MIN_PID 1
#define NUM_FILE_DESCRIPTORS 8
/* First four bytes of the executable, little endian uint32*/
#define EXECUTABLE_MAGIC 0x464c457f
/* Logical Address where process image is loaded */
#define PROCESS_LD_LOCATION 0x08048000
#define PROCESS_START_LOCATION 0x08000000
#define PROCESS_ESP_LOCATION 0x083FFFFC
#define PROCESS_EIP_LOCATION 24  // 24-27, entrypoint
#define PROCESS_HEADER_BLOCK_LENGTH 28  // Contains EIP and things

#define KERNEL_AREA_BOTTOM 0x800000  /* End of kernel memory: 8MB */
#define PROCESS_KERNEL_STACK_SIZE 0x2000  /* Per-process stack: 8kB */

#define FD_STDIN 0
#define FD_STDOUT 1

#define DRIVER_TERMINAL 0
#define DRIVER_RTC 1
#define DRIVER_FILE 2
#define DRIVER_DIR 3
// FIXME: Filesystem is excluded for the time being
#define DRIVER_COUNT 4

typedef struct {
    /* Ops table location for syscall on the open file */
    uint32_t operations_table;
    /* INode of the open file. 0 for devices */
    uint32_t inode;
    /* Position of the file cursor */
    uint32_t position;
    /* Flags = 1 means taken, vice versa */
    uint32_t flags;
} file_descriptor_t;

typedef struct {
    uint8_t in_use;  /* 0 = available, 1 = taken */
    uint8_t pid;  /* Process ID, 1 to 8 */
    uint32_t esp0;
    uint8_t parent_pid;
    int8_t args[SIZE_INPUT_BUFFER];  /* Argument string */
    file_descriptor_t file_descriptors[NUM_FILE_DESCRIPTORS];
    uint32_t parent_esp;
    uint32_t parent_ebp;
    uint32_t context_esp0;
    uint32_t context_esp;
    uint32_t context_ebp;
} pcb_t;

/* Quick access PCB locations for each process.
 * Since pid 0 doesn't exist, this is only for 1-8.
 */
extern pcb_t* processes[NUM_PROCESSES];

int32_t execute(const str command);
int32_t halt(uint8_t status);
void switch_to_process();

extern void init_pcb();

extern int sched_enable;
