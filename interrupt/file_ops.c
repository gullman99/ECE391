/**
 * File Operation syscall operations.
 */
#include "syscall.h"
#include "process.h"
#include "../lib.h"
#include "../driver/terminal.h"
#include "../driver/rtc.h"
#include "../filesystem.h"

typedef int32_t (*func_open)(const str);
typedef int32_t (*func_write)(int32_t, const void*, int32_t);
typedef int32_t (*func_read)(int32_t, void*, int32_t, int32_t);
typedef int32_t (*func_close)(int32_t);


func_open drivers_open[DRIVER_COUNT] = {terminal_open, rtc_open, file_open, directory_open};
func_close drivers_close[DRIVER_COUNT] = {terminal_close, rtc_close, file_close, directory_close};
func_read drivers_read[DRIVER_COUNT] = {terminal_read, rtc_read, file_read, directory_read};
func_write drivers_write[DRIVER_COUNT] = {terminal_write, rtc_write, file_write, directory_write};


int32_t read(int32_t fd, void *buf, int32_t nbytes) {
    // STEP 1: Invalidate bad FD, unused FD, stdout
    if (fd < 0 || fd >= NUM_FILE_DESCRIPTORS) return -1;
    if (fd == FD_STDOUT) return -1;
    file_descriptor_t* descriptor = &(processes[terminals[active_terminal].pid]->file_descriptors[fd]);
    if (!descriptor->flags) return -1;  // Not in use
    // STEP 2: Call the driver
    // TODO: Plan the rest for the FS.
    int32_t bytes_read = drivers_read[descriptor->operations_table](
        descriptor->inode, buf, nbytes, descriptor->position);
    if (bytes_read >= 0) descriptor->position += bytes_read;
    return bytes_read;
}

int32_t write(int32_t fd, const void *buf, int32_t nbytes) {
    // STEP 1: Invalidate bad FD, unused FD, stdin
    if (fd < 0 || fd >= NUM_FILE_DESCRIPTORS) return -1;
    if (fd == FD_STDIN) return -1;
    file_descriptor_t descriptor = processes[terminals[active_terminal].pid]->file_descriptors[fd];
    if (!descriptor.flags) return -1;  // Not in use
    // STEP 2: Call the driver
    // TODO: Plan the rest for the FS.
    return drivers_write[descriptor.operations_table](descriptor.inode, buf, nbytes);
}

int32_t open(const str filename) {
    int32_t result, fd;
    dentry_t curr_dentry;

    //find the dentry, error if not found
    if (read_dentry_by_name(filename, &curr_dentry) == -1) {return -1;}
    // Search for a file descriptor
    for (fd = FD_STDOUT+1; fd < NUM_FILE_DESCRIPTORS; fd++) {
        if (!processes[terminals[active_terminal].pid]->file_descriptors[fd].flags) {
            file_descriptor_t* target_fdesc = &(processes[terminals[active_terminal].pid]->file_descriptors[fd]);
            target_fdesc->flags = 1;
            target_fdesc->position = 0;
            uint32_t driver_type;
            switch (curr_dentry.file_type)
            {
                case RTC_FILE_TYPE: driver_type = DRIVER_RTC; break;
                case DIR_FILE_TYPE: driver_type = DRIVER_DIR; break;
                case REG_FILE_TYPE: driver_type = DRIVER_FILE; break;
                default: driver_type = DRIVER_TERMINAL;
            }
            target_fdesc->inode = curr_dentry.inode_num;
            target_fdesc->operations_table = driver_type;
            result = drivers_open[driver_type](filename);
            if (result) {
                target_fdesc->flags = 0;
                return -1;
            }
            return fd;
        }
    }
    // Couldn't find a FD? Bad.
    return -1;
}

int32_t close(int32_t fd) {
    // STEP 1: Invalidate bad FD, unused FD
    if (fd <= FD_STDOUT || fd >= NUM_FILE_DESCRIPTORS) return -1;
    file_descriptor_t* descriptor = &(processes[terminals[active_terminal].pid]->file_descriptors[fd]);
    if (!descriptor->flags) return -1;  // Not in use
    // STEP 2: Mark the descriptor as not in use
    descriptor->flags = 0;
    // STEP 3: Call the actual close function
    return drivers_close[descriptor->operations_table](fd);
}

int32_t getargs(str buf, int32_t nbytes) {
    if (!buf) return -1;
    if (!nbytes) return -1;
    strncpy(buf, processes[terminals[active_terminal].pid]->args, nbytes);
    return 0;
}
