#pragma once

#include "../lib.h"

#define NUM_TERMINALS 3
#define SIZE_INPUT_BUFFER 128

// BEGIN CP2.1
typedef struct {
  uint8_t pid;  // PID of the current running process
  char input_buffer[SIZE_INPUT_BUFFER];
  // char video_buffer[NUM_COLS * NUM_ROWS * NUM_BITS_PER_PIXEL];
  str video_buffer;
  uint8_t buffer_pos;
  uint8_t read_complete; // Flippped to 1 when a newline is read
  uint8_t cursor_x;
  uint8_t cursor_y;
} terminal_t;

extern int foreground_terminal;
extern int active_terminal;
extern int previous_terminal;
extern terminal_t terminals[NUM_TERMINALS];

// syscall methods
int32_t terminal_open(const str filename);
int32_t terminal_read(int32_t fd, void *buf, int32_t nbytes, int32_t offset);
int32_t terminal_write(int32_t fd, const void *buf, int32_t nbytes);
int32_t terminal_close(int32_t fd);

// utility methods for terminal
void terminal_init();
void switch_foreground_terminal(uint8_t target, uint8_t backup_current);
void switch_active_terminal(uint8_t target);
void terminal_handle_key(uint8_t key, uint8_t ctrl, uint8_t alt);

// END CP2.1

void backup_cursor(uint8_t target);
