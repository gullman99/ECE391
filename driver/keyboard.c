#include "keyboard.h"
#include "../i8259.h"
#include "../lib.h"

#define KEY_CTRL 29
#define KEY_SHIFT_L 42
#define KEY_SHIFT_R 54
#define KEY_ALT 56
#define KEY_CAPS 58
#define KEY_UP 0x80

volatile char ctrl_down = 0;
volatile char left_shift_down = 0;
volatile char right_shift_down = 0;
volatile char alt_down = 0;
volatile char caps_down = 0;

// REF http://www.osdever.net/bkerndev/Docs/keyboard.htm
unsigned char scancode_keymap[128] = {
    0,          0,          '1',        '2', '3',  '4', '5',
    '6',        '7',        '8',                   /* 9 */
    '9',        '0',        '-',        '=', '\b', /* Backspace */
    '\t',                                          /* Tab */
    'q',        'w',        'e',        'r',       /* 19 */
    't',        'y',        'u',        'i', 'o',  'p', '[',
    ']',        '\n', /* Enter key */
    0,                /* 29   - Control = 29 */
    'a',        's',        'd',        'f', 'g',  'h', 'j',
    'k',        'l',        ';', /* 39 */
    '\'',       '`',        0,   /* Left shift = 42 */
    '\\',       'z',        'x',        'c', 'v',  'b', 'n', /* 49 */
    'm',        ',',        '.',        '/', 0, /* Right shift = 54 */
    '*',        0,                              /* Alt = 56 */
    ' ',                                        /* Space bar */
    0,                                          /* Caps lock = 58 */
    KEY_FN,                                     /* 59 - F1 key ... > */
    KEY_FN + 1, KEY_FN + 2, KEY_FN + 3,  KEY_FN + 4,   KEY_FN + 5,    KEY_FN + 6,   KEY_FN + 7,
    KEY_FN + 8,          KEY_FN + 9,                         /* < ... F10 */
    0,                                     /* 69 - Num lock*/
    0,                                     /* Scroll Lock */
    0,                                     /* Home key */
    0,                                     /* Up Arrow */
    0,                                     /* Page Up */
    '-',        0,                         /* Left Arrow */
    0,          0,                         /* Right Arrow */
    '+',        0,                         /* 79 - End key*/
    0,                                     /* Down Arrow */
    0,                                     /* Page Down */
    0,                                     /* Insert Key */
    0,                                     /* Delete Key */
    0,          0,          0,          0, /* F11 Key */
    0,                                     /* F12 Key */
    0,                                     /* All other keys are undefined */
};

// A different scan map to use when caps / shift
unsigned char scancode_keymap_caps[128] = {
    0,          0,          '!',        '@', '#',  '$', '%',
    '^',        '&',        '*',                   /* 9 */
    '(',        ')',        '_',        '+', '\b', /* Backspace */
    '\t',                                          /* Tab */
    'Q',        'W',        'E',        'R',       /* 19 */
    'T',        'Y',        'U',        'I', 'O',  'P', '{',
    '}',        '\n', /* Enter key */
    0,                /* 29   - Control = 29 */
    'A',        'S',        'D',        'F', 'G',  'H', 'J',
    'K',        'L',        ':', /* 39 */
    '\'',       '~',        0,   /* Left shift = 42 */
    '\\',       'Z',        'X',        'C', 'V',  'B', 'N', /* 49 */
    'M',        '<',        '>',        '?', 0, /* Right shift = 54 */
    '*',        0,                              /* Alt = 56 */
    ' ',                                        /* Space bar */
    0,                                          /* Caps lock = 58 */
    KEY_FN,                                     /* 59 - F1 key ... > */
    KEY_FN + 1, KEY_FN + 2, KEY_FN + 3, KEY_FN + 4,   KEY_FN + 5,    KEY_FN + 6,   KEY_FN + 7,
    KEY_FN + 8,          KEY_FN + 9,                         /* < ... F10 */
    0,                                     /* 69 - Num lock*/
    0,                                     /* Scroll Lock */
    0,                                     /* Home key */
    0,                                     /* Up Arrow */
    0,                                     /* Page Up */
    '-',        0,                         /* Left Arrow */
    0,          0,                         /* Right Arrow */
    '+',        0,                         /* 79 - End key*/
    0,                                     /* Down Arrow */
    0,                                     /* Page Down */
    0,                                     /* Insert Key */
    0,                                     /* Delete Key */
    0,          0,          0,          0, /* F11 Key */
    0,                                     /* F12 Key */
    0,                                     /* All other keys are undefined */
};

/* void keyboard_init(void)
 * Inputs: none
 * Return Value: none
 * Function: initialize the keyboard device */
void keyboard_init(void) {
  enable_irq(KB_IRQNUM);
  return;
}

/* uint8_t get_keyboard_input (void)
 * Inputs: none
 * Return Value: one byte scancode read from keyboard
 * Function: read in a 8bit scancode from the keyboard, used
 *           in keyboard interrupt handler.*/
uint8_t get_keyboard_input(void) {
  uint8_t data;
  data = inb(KB_PORT); // get key press scan code
  if (data & KEY_UP) {
    // Handle release key
    data &= ~KEY_UP;
    switch (data) {
    case KEY_CTRL:
      ctrl_down = 0;
      break;
    case KEY_ALT:
      alt_down = 0;
      break;
    case KEY_SHIFT_L:
      left_shift_down = 0;
      break;
    case KEY_SHIFT_R:
      right_shift_down = 0;
      break;
    }
    return 0;
  } else {
    // Handle key down
    switch (data) {
    case KEY_CTRL:
      ctrl_down = 1;
      break;
    case KEY_ALT:
      alt_down = 1;
      break;
    case KEY_SHIFT_L:
      left_shift_down = 1;
      break;
    case KEY_SHIFT_R:
      right_shift_down = 1;
      break;
    case KEY_CAPS:
      caps_down = ~caps_down;
      break;
    }
    // If caps down and one of the shift is down, gives 0
    // If caps up and one of the shift is down, gives 1
    return (caps_down ^ (left_shift_down || right_shift_down))
               ? scancode_keymap_caps[data]
               : scancode_keymap[data];
  }
}
