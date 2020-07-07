/**
 * Interrupt, syscall and handler Vectors
 * Also error messages are defined here
 */

#pragma once

/* Conventions:
 * - VEC = Vector (syscall, interrupts)
 * - EXC = Exception
 */

#define VEC_SYSCALL 0x80
#define VEC_TIMER 0x20
#define VEC_KEYBOARD 0x21
#define VEC_RTC 0x28

// Ignoring 1, 9, 15, which are Intel reserved.
#define EXC_DIVIDE 0
#define EXC_NMI 2
#define EXC_BREAKPOINT 3
#define EXC_OVERFLOW 4
#define EXC_BOUND 5
#define EXC_INVALID_OP 6
#define EXC_DEVICE_NOT_AVAIL 7
#define EXC_DOUBLE_FAULT 8
#define EXC_INVALID_TSS 10
#define EXC_SEG_NOT_PRESENT 11
#define EXC_STACK_SEGFAULT 12
#define EXC_GP_FAULT 13
#define EXC_PAGE_FAULT 14
#define EXC_ASSERTION_FAILURE 15
#define EXC_MATH_FAULT 16
#define EXC_ALIGNMENT_FAULT 17
#define EXC_MACHINE_ABORT 18

#define LAST_EXC 18 // Position of last exception
#define TOTAL_EXC 32
