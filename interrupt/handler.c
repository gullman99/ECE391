#include "handler.h"
#include "process.h"
#include "../driver/keyboard.h"
#include "../driver/rtc.h"
#include "../driver/terminal.h"
#include "../interrupt/process.h"
#include "../i8259.h"
#include "../lib.h"
#include "vectors.h"

static const char *exception_messages[LAST_EXC + 1] = {
    "Division Error",
    "Intel Reserved Exception 1",
    "Non-maskable Interrupt",
    "Breakpoint (INT 3)",
    "Overflow",
    "BOUND Range Exceeded",
    "Invalid Opcode",
    "Device Not Available (No Math Coprocessor)",
    "Double Fault",
    "Intel Reserved Exception 9",
    "Invalid TSS",
    "Segment Not Present",
    "Stack Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "ECE 391 Assertion Failure",
    "x87 FPU Float Point Error (Math Fault)",
    "Alignment Fault",
    "Machine Abort"};

// BEGIN CP1.4 Initialize Devices

/**
 * Timer handler
 * INPUT: None.
 * OUTPUT: None.
 * EFFECT: Handles timer chip interrupt.
 */
void handle_timer() {
  cli();
  send_eoi(0);
  if (sched_enable) {
    int target_terminal;
    target_terminal = (active_terminal+1)%NUM_TERMINALS;
    switch_active_terminal(target_terminal);
  }
  sti();
}

/**
 * Keyboard handler
 * INPUT: None.
 * OUTPUT: None.
 * EFFECT: Handles keyboard interrupt.
 */
void handle_keyboard() {
  cli();
  send_eoi(KB_IRQNUM);
  char c = get_keyboard_input();
  terminal_handle_key(c, ctrl_down, alt_down);
  sti();
}

/**
 * RTC handler
 * INPUT: None.
 * OUTPUT: None.
 * EFFECT: Handles RTC interrupt.
 */
void handle_rtc() {
  cli();
  outb(0x0C, RTC_PORT);  // select registers C
  inb(RTC_CMOS_PORT);    // discard value, allow another irq to be genereate
  test_rtc_ticks_incr(); // increments rtc test tick counter if enabled
  rtc_interrupt_recieved();
  send_eoi(RTC_IRQNUM);
  sti();
}

// END CP1.4

/**
 * Exception handler for a given vector number.
 * INPUT: None.
 * OUTPUT: None.
 * EFFECT: Handles syscall.
 */
void handle_exception(uint8_t vector_no) {
  // TODO: Display a BSOD for the given interrupt vector
  printf("EXCEPTION\n");
  printf("Vector #%d: %s\n", vector_no, exception_messages[vector_no]);
  halt(255);
}

/**
 * Default Interrupt Handler (for undefined IDT entries)
 * INPUT: None.
 * OUTPUT: None.
 * EFFECT: Handles syscall.
 */
void default_interrupt() {
  cli();
  printf("Undefined interrupt!\n");
  sti();
}
