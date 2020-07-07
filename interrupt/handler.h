/**
 * All exception handler functions go here.
 * ONLY the handlers - Indirection functions elsewhere. E.g. Syscalls.
 */

#pragma once

#include "../types.h"

extern void handle_timer();
extern void handle_keyboard();
extern void handle_rtc();
extern void handle_exception(uint8_t vector_no);

extern void keyboard_isr();
extern void rtc_isr();
extern void timer_isr();

extern void default_interrupt();

// Now, since handle_exception requires a parameter, but exceptor handler cannot
// take a parameter, we allow the table generation code to declare its own
// "partials". Each of which will spawn its own function.
#define HANDLE_EXC_FUNCTION_NAME(VEC_NO) handle_exc_##VEC_NO
#define DEF_HANDLE_EXC_PARTIAL(VEC_NO)                                         \
  void handle_exc_##VEC_NO() { handle_exception(VEC_NO); }

// Since these partials must be used in-situ, we declare them
// at table.c. Can't do it here or in handler.c because the macros
// only contain definitions.
