#include "table.h"

#include "../types.h"    // Import the uint32_t and stuff
#include "../x86_desc.h" // Import idt_desc_t and NUM_VEC
#include "handler.h"     // Interrupt handlers
#include "syscall.h"     // syscallhandler
#include "vectors.h"     // Interrupt vector magic numbers

// BEGIN CP1.3
// Define all the exception handlers here.
// For the reasoning behind this, see handler.h:24
DEF_HANDLE_EXC_PARTIAL(EXC_DIVIDE)
DEF_HANDLE_EXC_PARTIAL(EXC_NMI)
DEF_HANDLE_EXC_PARTIAL(EXC_BREAKPOINT)
DEF_HANDLE_EXC_PARTIAL(EXC_OVERFLOW)
DEF_HANDLE_EXC_PARTIAL(EXC_BOUND)
DEF_HANDLE_EXC_PARTIAL(EXC_INVALID_OP)
DEF_HANDLE_EXC_PARTIAL(EXC_DEVICE_NOT_AVAIL)
DEF_HANDLE_EXC_PARTIAL(EXC_DOUBLE_FAULT)
DEF_HANDLE_EXC_PARTIAL(EXC_INVALID_TSS)
DEF_HANDLE_EXC_PARTIAL(EXC_SEG_NOT_PRESENT)
DEF_HANDLE_EXC_PARTIAL(EXC_STACK_SEGFAULT)
DEF_HANDLE_EXC_PARTIAL(EXC_GP_FAULT)
DEF_HANDLE_EXC_PARTIAL(EXC_PAGE_FAULT)
DEF_HANDLE_EXC_PARTIAL(EXC_ASSERTION_FAILURE)
DEF_HANDLE_EXC_PARTIAL(EXC_MATH_FAULT)
DEF_HANDLE_EXC_PARTIAL(EXC_ALIGNMENT_FAULT)
DEF_HANDLE_EXC_PARTIAL(EXC_MACHINE_ABORT)

/**
 * Initializes the IDT Table.
 *
 * INPUT: none
 * OUTPUT: none
 * SIDE EFFECT: Populates the IDT Table.
 */
void init_idt() {
  int i;
  for (i = 0; i < NUM_VEC; i++) {
    /* Setup Checklist:
     * - seg_selector: KERNEL_CS
     * - Reserved 4 = 0
     * - Reserved123 = 110 for INT, 111 for EXC
     * - Size = 1
     * - dpl = 0, unless syscall = 3 (Call from user)
     * - present = 1
     * - Offsets no change
     */
    idt[i].seg_selector = KERNEL_CS;
    idt[i].reserved4 = 0;
    idt[i].reserved3 = ((i < TOTAL_EXC) || (i == VEC_SYSCALL));
    idt[i].reserved2 = 1;
    idt[i].reserved1 = 1;
    idt[i].size = 1;
    idt[i].dpl = (i == VEC_SYSCALL) ? 3 : 0;
    idt[i].present = 1;
    SET_IDT_ENTRY(idt[i], default_interrupt);
  }
  // Set IDT for Handlers
  SET_IDT_ENTRY(idt[VEC_SYSCALL], handle_syscall);
  SET_IDT_ENTRY(idt[VEC_TIMER], timer_isr);
  SET_IDT_ENTRY(idt[VEC_KEYBOARD], keyboard_isr);
  SET_IDT_ENTRY(idt[VEC_RTC], rtc_isr);
  // Set IDT for Exception Handlers
  // No handler for reserved exc.
  SET_IDT_ENTRY(idt[EXC_DIVIDE], HANDLE_EXC_FUNCTION_NAME(EXC_DIVIDE));
  SET_IDT_ENTRY(idt[EXC_NMI], HANDLE_EXC_FUNCTION_NAME(EXC_NMI));
  SET_IDT_ENTRY(idt[EXC_BREAKPOINT], HANDLE_EXC_FUNCTION_NAME(EXC_BREAKPOINT));
  SET_IDT_ENTRY(idt[EXC_OVERFLOW], HANDLE_EXC_FUNCTION_NAME(EXC_OVERFLOW));
  SET_IDT_ENTRY(idt[EXC_BOUND], HANDLE_EXC_FUNCTION_NAME(EXC_BOUND));
  SET_IDT_ENTRY(idt[EXC_INVALID_OP], HANDLE_EXC_FUNCTION_NAME(EXC_INVALID_OP));
  SET_IDT_ENTRY(idt[EXC_DEVICE_NOT_AVAIL],
                HANDLE_EXC_FUNCTION_NAME(EXC_DEVICE_NOT_AVAIL));
  SET_IDT_ENTRY(idt[EXC_DOUBLE_FAULT],
                HANDLE_EXC_FUNCTION_NAME(EXC_DOUBLE_FAULT));
  SET_IDT_ENTRY(idt[EXC_INVALID_TSS],
                HANDLE_EXC_FUNCTION_NAME(EXC_INVALID_TSS));
  SET_IDT_ENTRY(idt[EXC_SEG_NOT_PRESENT],
                HANDLE_EXC_FUNCTION_NAME(EXC_SEG_NOT_PRESENT));
  SET_IDT_ENTRY(idt[EXC_STACK_SEGFAULT],
                HANDLE_EXC_FUNCTION_NAME(EXC_STACK_SEGFAULT));
  SET_IDT_ENTRY(idt[EXC_GP_FAULT], HANDLE_EXC_FUNCTION_NAME(EXC_GP_FAULT));
  SET_IDT_ENTRY(idt[EXC_PAGE_FAULT], HANDLE_EXC_FUNCTION_NAME(EXC_PAGE_FAULT));
  SET_IDT_ENTRY(idt[EXC_ASSERTION_FAILURE],
                HANDLE_EXC_FUNCTION_NAME(EXC_ASSERTION_FAILURE));
  SET_IDT_ENTRY(idt[EXC_MATH_FAULT], HANDLE_EXC_FUNCTION_NAME(EXC_MATH_FAULT));
  SET_IDT_ENTRY(idt[EXC_ALIGNMENT_FAULT],
                HANDLE_EXC_FUNCTION_NAME(EXC_ALIGNMENT_FAULT));
  SET_IDT_ENTRY(idt[EXC_MACHINE_ABORT],
                HANDLE_EXC_FUNCTION_NAME(EXC_MACHINE_ABORT));
  // Final step: Load the IDT
  lidt(idt_desc_ptr);
}

// END CP1.3
