#include "rtc.h"
#include "../i8259.h"
#include "../lib.h"

// vars for testings
static uint32_t test_ticks;
int test_flag;
volatile uint8_t interrupt_recieved;

/*lookup table get frequency codes*/
const uint8_t RATE_TABLE[15] = {
    0x0F, // 2Hz
    0x0E, // 4Hz
    0x0D, // 8Hz
    0x0C, // 16Hz
    0x0B, // 32Hz
    0x0A, // 64Hz
    0x09, // 128Hz
    0x08, // 256Hz
    0x07, // 512Hz
    0x06, // 1024Hz
    0x05, // 2048Hz
    0x04, // 4096Hz
    0x03, // 8192Hz
    0x02, // 128Hz
    0x01  // 256Hz
};

/* void rtc_init(void)
 * Inputs: none
 * Return Value: none
 * Function: initializes the RTC device */
void rtc_init(void) {

  // REF OSDev https://wiki.osdev.org/RTC, Dallas Semiconductor DS12887 Real
  // Time Clock datasheet
  cli();

  test_flag = 0; // turns off testing ticks

  /* setting RTC A register */
  outb(RTC_A_REG, RTC_PORT);
  unsigned char a_old = inb(RTC_CMOS_PORT);
  outb((0x80 & a_old) | 0x2F,
       RTC_CMOS_PORT); // setting DV2-0 to 010 to turn on oscillator, clears all
                       // other bits besides UIP bit

  /* setting RTC B register */
  outb(RTC_B_REG, RTC_PORT);
  unsigned char b_old = inb(RTC_CMOS_PORT);
  outb((0x0F & b_old) | 0x40,
       RTC_CMOS_PORT); // Sets PIE bit to one, enables square wave, binary
                       // calendar data, 24 hour mode, and daylight savings

  enable_irq(RTC_IRQNUM); // enables the RTC irq line on the PIC
  sti();
}

/* rtc_enable_period_irq
 * Inputs: none
 * Return Value: none
 * Function: enables periodic irqs*/
void rtc_enable_period_irq(void) {
  cli(); // disable interrupts

  /* setting RTC B register */
  outb(RTC_B_REG, RTC_PORT);
  unsigned char b_old = inb(RTC_CMOS_PORT);
  outb(b_old | 0x40,
       RTC_CMOS_PORT); // Sets PIE bit to one (enables periodic interrupts)

  sti();
}

/* rtc_disable_period_irq
 * Inputs: none
 * Return Value: none
 * Function: disables the periodic irqs */
void rtc_disable_period_irq(void) {
  cli(); // disable interrupts

  /* setting RTC B register */
  outb(RTC_B_REG, RTC_PORT);
  unsigned char b_old = inb(RTC_CMOS_PORT);
  outb(b_old & 0xBF,
       RTC_CMOS_PORT); // Sets PIE bit to zero (disables periodic interrupts)

  sti();
}

/* rtc_set_rate
 * Inputs: integer representing desired frequency in hertz, must be power of two
 * Return Value: 0 on success, -1 on failure (invalid frequency arg)
 * Function: sets the frequency of the RTC's periodic interrupts */
int rtc_set_rate(uint16_t rate) {
  cli();

  uint8_t rate_code = 0xFF;
  int power = 0;

  uint16_t bitmask = 0x2;
  if ((rate == 0)) {
    return -1;
  } // if rate is zero, return error
  while ((bitmask & rate) == 0) {
    power++;
    bitmask = bitmask << 1;
  }

  if ((rate & (~bitmask))) {
    return -1;
  }                              // if a non power of two, return error
  rate_code = RATE_TABLE[power]; // get corrosponding rate code from our table

  outb(RTC_A_REG, RTC_PORT);             // swtich to rtc A register
  outb(rate_code | 0x40, RTC_CMOS_PORT); // Sets rate using our rate code
  sti();

  return 0; // return success
}

/* rtc_interrupt_recieved
 * Inputs: none
 * Return Value: none
 * Function: sets the recieved interrupt flag. for use by the RTC interrupt
 * handler*/
void rtc_interrupt_recieved(void) {
  interrupt_recieved = 1;
  // doesnt need cli/sti because is only called from rtc handler which is
  // already protected.
}

/* rtc_open
 * Inputs: filename -- name of RTC file to open
 * Return Value: returns a file descriptor on success, -1 on failure
 * Function: opens an RTC file*/
int32_t rtc_open(const str filename) { return rtc_set_rate(2); }

/* rtc_read
 * Inputs: fd -- an RTC file descriptor
 *         buf -- A pointer to a buffer, not used here
 *         nbytes -- the number of bytes to read, not used here
 * Return Value: 0 on success, -1 on failure
 * Function: This function "spinlocks" until another RTC interrupt is recieved,
 * which creates a sleep effect with the length based on the RTC frequency*/
int32_t rtc_read(int32_t fd, void *buf, int32_t nbytes, int32_t offset) {
  cli();
  interrupt_recieved = 0;
  sti();
  while (1) { // waits until a single rtc interrupt has been recieved
    if (interrupt_recieved) {
      break;
    }
  }
  return 0;
}

/* rtc_write
 * Inputs: fd -- an RTC file descriptor
 *         buf -- A pointer to a buffer, should contain an int representing the
 * desired hz power of two. nbytes -- the number of bytes to read, should always
 * be 4 Return Value: 0 on success, -1 on failure (invalid freqeuncy)
 * Function: sets the frequency of the RTC, up to 1024Hz */
int32_t rtc_write(int32_t fd, const void *buf, int32_t nbytes) {
  cli();
  if (nbytes != 4 || buf == 0) {
    return -1;
  } // should only accept a 4 byte arg and non null pointer
  int32_t rate =
      *((const int32_t *)buf); // get the 4 byte arg from provided pointer
  if (rate > 1024) {
    return -1;
  } // user calls are restricted to 1024Hz
  if (rtc_set_rate(rate) == 0) {
    return 4;
  } // if success, return 4
  sti();
  return -1; // else return error
}

/* rtc_close
 * Inputs: fd -- file descriptor of an RTC file
 * Return Value: 0
 * Function: closes an open RTC file */
int32_t rtc_close(int32_t fd) { return 0; }

/***** TEST FUNCTIONS *****/

/* void test_rtc_enable(void)
 * Inputs: none
 * Return Value: none
 * Function: enables incrementing of test tick counter, resets to zero */
void test_rtc_enable(void) {
  test_flag = 1;  // turns on testing ticks
  test_ticks = 0; // sets test tick to zero
}

/* void test_rtc_disable(void)
 * Inputs: none
 * Return Value: none
 * Function: disables incrementing of test tick counter, resets to zero */
void test_rtc_disable(void) {
  test_flag = 0;  // turns off testing ticks
  test_ticks = 0; // sets test tick to zero
}

/* void test_rtc_ticks_incr(void)
 * Inputs: none
 * Return Value: none
 * Function: Increments the test tick counter if enabled*/
void test_rtc_ticks_incr(void) {
  if ((test_ticks < 0xFFFFFFFF) && test_flag) {
    test_ticks++;
  }
}

/* uint32_t test_rtc_get_ticks(void)
 * Inputs: none
 * Return Value: number of ticks elapsed since reset
 * Function: get the current value of the test tick counter*/
uint32_t test_rtc_get_ticks(void) { return test_ticks; }
