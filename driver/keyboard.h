/* keyboard.h - contains initialization functionality for Real Time Clock
 */

#ifndef _KEYBOARD_H
#define _KEYBOARD_H

#include "../types.h"

/*IRQ line number that corrosponds to the keyboard*/
#define KB_IRQNUM 1

/* ports for reading data from keyboard and getting its status */
#define KB_PORT 0x60
#define KB_STATUS_PORT 0x64
// BEGIN CP1.4

// BEGIN CP2.1
#define KEY_ACCEPTED_MIN 4  // Least key that can be printed
#define KEY_ACCEPTED_MAX 127 // Max key that can be printed
#define KEY_FN 130
// END

extern volatile char ctrl_down;
extern volatile char left_shift_down;
extern volatile char right_shift_down;
extern volatile char alt_down;
extern volatile char caps_down;

/* function to initialize the RTC */
void keyboard_init(void);

/* function to read in scancode from keyboard */
uint8_t get_keyboard_input(void);

// END  CP1.4

#endif /* _KEYBOARD_H */
