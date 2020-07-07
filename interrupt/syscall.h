/**
 * All Syscall functions go here.
 */

#pragma once

#include "../types.h" // For int32_t etc.

extern void handle_syscall(); //assembly syscall linkages

extern int32_t execute(const str command);
extern int32_t halt(uint8_t status);
extern int32_t read(int32_t fd, void *buf, int32_t nbytes);
extern int32_t write(int32_t fd, const void *buf, int32_t nbytes);
extern int32_t open(const str filename);
extern int32_t close(int32_t fd);
extern int32_t getargs(str buf, int32_t nbytes);
//extern int32_t vidmap(str *screen_start);
extern int32_t map_addr_video_memory(uint8_t ** screen_start);

// Extra Credit
extern int32_t set_handler(uint32_t signum, void *handler_address);
extern int32_t sigreturn(void);
