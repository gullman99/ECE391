#include "terminal.h"
#include "../lib.h"
#include "keyboard.h"
#include "../interrupt/syscall.h"
#include "../interrupt/process.h"
#include "../paging.h"

terminal_t terminals[NUM_TERMINALS];

void set_terminal_vmem(uint8_t target);

/**
 * Foreground terminal is the terminal actually shown to user and accepting user input.
 * That means, if switching to foreground terminal, the video virtual address must point
 * at the actual video memory
 */
int foreground_terminal = 0;

/**
 * Active terminal is the terminal whose active process is being executed.
 * If active terminal is not the foreground, its virtual address must point
 * at its offscreen video memory buffer.
 */
int active_terminal = 0;

/* Temporary variable that saves what terminal we are switching away from,
 * used during process switching.
 */
int previous_terminal = 0;

// BEGIN CP2.1

/* Syscall interface for opening a terminal
 * Inputs: None
 * Return value: zero
 * Side effect: none
 */
int32_t terminal_open(const str filename) { return 0; }

/* Syscall interface for reading line from terminal til Enter key
 * Inputs: - buf: pointer to your buffer
 *         - nbytes: max number of bytes to read
 * Return value: Actual number of chars read
 * Side effect: After read, terminal buffer is cleared.
 */
int32_t terminal_read(int32_t fd, void *buf, int32_t nbytes, int32_t offset) {
  cli();
  terminal_t *cur_term = &(terminals[active_terminal]);
  sti();
  int i;
  char *charbuf = (char *)buf;
  // Now we wait for the terminal to complete a line of input
  while (!cur_term->read_complete)
    ;
  //puts("Done read\n");
  // If asking for more than we have, we truncate it
  if (nbytes >= cur_term->buffer_pos)
    nbytes = cur_term->buffer_pos;
  // We could have used strncpy but we want a length here..
  // So reinventing the wheel.
  // Also this is a contingency plan if str is corrupted..
  for (i = 0; i < nbytes; i++)
    charbuf[i] = cur_term->input_buffer[i];
  // Wrapup: Clear the terminal buffer
  cur_term->read_complete = 0;
  cur_term->buffer_pos = 0;
  cur_term->input_buffer[0] = 0;
  return i;
}

/* Syscall interface for writing string to terminal
 * Inputs: - buf: pointer to string to write
 *         - nbytes: Number of bytes to write
 * Return value: Actual number of chars written
 * Side effect: String put into the buffer
 */
int32_t terminal_write(int32_t fd, const void *buf, int32_t nbytes) {
  cli();
  int i;
  char *charbuf = (char *)buf;
  for (i = 0; i < nbytes; i++) {
    // String ends? I'll just end.
    if (!charbuf[i])
      break;
    putc(charbuf[i]);
  }
  backup_cursor(active_terminal);
  if (foreground_terminal == active_terminal) set_cursor();
  sti();
  return i; // Number of bytes written
}

/* Syscall interface for closing a terminal
 * Inputs: None
 * Return value: zero
 * Side effect: none
 */
int32_t terminal_close(int32_t fd) { return 0; }

/* Initializes all terminals, and put the first one on screen
 * Inputs: None
 * Return value: none
 * Side effect: All terminals buffers cleared and inited
 */
void terminal_init() {
  int i;
  for (i = 0; i < NUM_TERMINALS; i++) {
    terminals[i].video_buffer = (str)interactive_term_addr(i);
    // Initialize the buffers
    int j;
    for (j = 0; j < NUM_COLS * NUM_ROWS; j++) {
      terminals[i].video_buffer[j << 1] = ' ';
      terminals[i].video_buffer[(j << 1) + 1] = ATTRIB;
    }
    for (j = 0; j < SIZE_INPUT_BUFFER; j++)
      terminals[i].input_buffer[j] = 0;
    // Initialize the states
    terminals[i].buffer_pos = 0;    // Resets the buffer write head
    terminals[i].read_complete = 0; // Resets the completion flag
    // Initialize the cursor
    terminals[i].cursor_x = 0;
    terminals[i].cursor_y = 0;
    // Initialize the active process
    terminals[i].pid = NULL;
  }
  foreground_terminal = 0; // Set first terminal active
  active_terminal = 0;
  clear();
 switch_foreground_terminal(0, 0);
}

/* Switch to a different foreground terminal.
 * Inputs: - target: Target terminal number
 *         - backup_current: Save current video buffer. Should be 1!
 * Return value: none
 * Side effect: Prev terminal screen saved, recalling next terminal
 */
void switch_foreground_terminal(uint8_t target, uint8_t backup_current) {
  // Don't do this if asking to switch to the same terminal
  if (backup_current && target == foreground_terminal)
    return;
  // First map the video memory back to video to operate on it
  switch_vid_address(VIDEO);
  if (backup_current) {
    terminal_t *old_term = &(terminals[foreground_terminal]);
    memcpy(old_term->video_buffer, (char *)VIDEO,
           NUM_COLS * NUM_ROWS * NUM_BITS_PER_PIXEL);
    //old_term->cursor_x = screen_x;
    //old_term->cursor_y = screen_y;
  }
  // Switch to current terminal
  foreground_terminal = target;
  terminal_t *new_term = &(terminals[target]);
  memcpy((char *)VIDEO, new_term->video_buffer,
         NUM_COLS * NUM_ROWS * NUM_BITS_PER_PIXEL);
  screen_x = new_term->cursor_x;
  screen_y = new_term->cursor_y;
  set_cursor();
  // Map active memory back
  set_terminal_vmem(active_terminal);
}


/* Remap to a different foreground terminal's buffer.
 * Inputs: - target: Target terminal number
 * Return value: none
 * Side effect: Change screen_start to the new terminal
 */
void set_terminal_vmem(uint8_t target) {
	// Remap virtual cursor
	screen_x = terminals[target].cursor_x;
	screen_y = terminals[target].cursor_y;
	// Remap the video memory
	  if (active_terminal == foreground_terminal) {
		// Map physical video in
		switch_vid_address(VIDEO);
	  } else {
		// Map one of the offscreen buffers
		switch_vid_address(nonactive_terms[active_terminal]);
	  }
	if (active_terminal == foreground_terminal) set_cursor();
}

/* Backup cursor location to that terminal..
 * Inputs: - target: Target terminal number
 * Return value: none
 * Side effect: Cursor location backed up
 */
void backup_cursor(uint8_t target) {
	terminals[target].cursor_x = screen_x;
	terminals[target].cursor_y = screen_y;
}

/* switch_active_terminal(uint8_t target)
 * Changes the current active terminal and triggers a process switch to it. 
 * Should only be called from the task scheduler.
 * Input: target - terminal which should be switched to
 * Outputs: modifies active_terminal and previous_terminal vars
 * Returns: none
 */
void switch_active_terminal(uint8_t target) {
  if (target == active_terminal) {return;}
  previous_terminal = active_terminal;
  active_terminal = target;
  set_terminal_vmem(target);
  switch_to_process();
}
/* Push the character to the terminal buffer
 * Inputs: - term: Pointer to current terminal
 *         - ch: Character to put in
 * Return value: 0 if buffer is full and can't be advanced. OTherwise 1
 * Side effect: Populates char to buffer if it fits.
 */
uint8_t terminal_advance_buffer(terminal_t *term, char ch) {
  cli();
  uint8_t result = 1;
  // CASE 1: If the buffer is complete, more keystrokes can screw it
  if (term->read_complete) {
    term->input_buffer[0] = 0; // Clear the string
    term->read_complete = 0;   // Reset the buffer
    term->buffer_pos = 0;
  }
  // Case 2: If it's a backspace, go back in buffer unless empty
  if (ch == '\b') {
    if (term->buffer_pos) {
      term->input_buffer[term->buffer_pos-1] = 0;
      term->buffer_pos--;
    } else {
      result = 0;
    }
    return result;
  }
  // CASE 3: Only add to buffer if input buffer is not full
  if (term->buffer_pos < SIZE_INPUT_BUFFER) {
    term->input_buffer[term->buffer_pos] = ch;
    term->buffer_pos++;
  } else {
    // If the last char is newline, let it go
    // Otherwise block screen printing
    if (ch != '\n') result = 0;
  }
  // CASE 4: If the current char is newline, make it complete
  if (ch == '\n')
    term->read_complete = 1;
  sti();
  return result;
}

// END CP2.1

/* Handling keystroke or key combo
 * Inputs: - key: Character assigned to key
 *         - ctrl: Boolean if ctrl is held down
 *         - alt: Boolean if alt is held down
 * Return value: none
 * Side effect: Prints char or executes function assigned to key
 */
void terminal_handle_key(uint8_t key, uint8_t ctrl, uint8_t alt) {
  // This is the current terminal
  terminal_t *fg_term = &(terminals[foreground_terminal]);
  //terminal_t *active_term = &(terminals[active_terminal]);
  // BEGIN CP2.1
  // Case 1: Ctrl+L: Clear screen and exit
  if (ctrl) {
    switch (key) {
    case 'l':
      clear();
      break;
    case 'L':
      clear();
      break;
    }
    return;
  }
  if (alt) {
    // BEGIN CP5.1
    // Warning: Terminal switching does not work on the Mac.
    // Hotkeys will invoke MacOS system config utils.
    if (key >= KEY_FN && key < KEY_FN + NUM_TERMINALS) {
      uint8_t target_terminal = key - KEY_FN;
      switch_foreground_terminal(target_terminal, 1);
      /* Following code is only used if scheduler is disabled. Only runs processes in the foreground
       * terminal, which is changed using ALT + F1-F3. Roughly the same as switch_active_terminal() */
      if (sched_enable != 1) {
		switch_active_terminal(target_terminal);
        //previous_terminal = active_terminal;
        //active_terminal = target_terminal;
        //asm volatile ("call switch_process_debug"); // calls a dummy isr to simulate interrupt procedure
      }
    }

    // END CP5.1
    return;
  }
  // Default case: Regular character, store this thing
  if ((key >= KEY_ACCEPTED_MIN && key <= KEY_ACCEPTED_MAX) || key == '\b') {
    // Before we putc: Make sure we map the screen
	  set_terminal_vmem(foreground_terminal);
    switch_vid_address(VIDEO);
    if(terminal_advance_buffer(fg_term, key)) {
		putc(key);
		backup_cursor(foreground_terminal);
		if (foreground_terminal == active_terminal) set_cursor();
	}
    // Tidy the memory back
    set_terminal_vmem(active_terminal);
  }
  // END CP2.1
}
