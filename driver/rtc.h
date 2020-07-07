/* rtc.h - contains initialization functionality for Real Time Clock
 */

#ifndef _RTC_H
#define _RTC_H

#include "../types.h"

/*IRQ line number that corrosponds to the keyboard*/
#define RTC_IRQNUM 8
#define RTC_PORT 0x70
#define RTC_CMOS_PORT 0x71

#define RTC_A_REG 0x8A
#define RTC_B_REG 0x8B
#define RTC_C_REG 0x8C
// BEGIN CP1.4

// switch (rate)
// {
//     for
//     case 2: rate_code = 1111;
//     case 4: rate_code = 1110;
//     case 8: rate_code = 1101;
//     case 16: rate_code = 1100;
//     case 16: rate_code = 1100;
//     case : rate_code = 1100;
//     case 16: rate_code = 1100;
// }

/* function to initialize the RTC */
void rtc_init(void);

/* enable periodic interrupts from the RTC */
void rtc_enable_period_irq(void);

/* disable periodic interrupts from the RTC */
void rtc_disable_period_irq(void);

/* change RTC interrupt frequency rate to a power of two*/
int rtc_set_rate(uint16_t rate);

/* sets the received interrupt flag */
void rtc_interrupt_recieved(void);

// open, read, write, and close

/* The call should find the directory entry corresponding to the
named file, allocate an unused file descriptor, and set up any data necessary to
handle the given type of file (directory, RTC device, or regular file). */
int32_t rtc_open(const str filename);

/* this call should always return 0, but only after an interrupt has
occurred (set a flag and wait until the interrupt handler clears it, then return
0). You should use a jump table referenced by the task’s file array to call from
a generic handler for this call into a file-type-specific function. This jump
table should be inserted into the file array on the open system call (see
below).
 */
int32_t rtc_read(int32_t fd, void *buf, int32_t nbytes, int32_t offset);

/* the system call should always accept only a 4-byte
integer specifying the interrupt rate in Hz, and should set the rate of periodic
interrupts accordingly. Writes to regular files should always return -1 to
indicate failure since the file system is read-only. The call returns the number
of bytes written, or -1 on failure. rate should only go up to 8192 Hz. Your
kernel should limit this further to 1024 Hz */
int32_t rtc_write(int32_t fd, const void *buf, int32_t nbytes);

/* closes the specified file descriptor and makes it available for return from
later calls to open. You should not allow the user to close the default
descriptors (0 for input and 1 for output). Trying to close an invalid
descriptor should result in a return value of -1; successful closes should
return 0.
 */
int32_t rtc_close(int32_t fd);

/* RTC VIRTUALIZATION NOTES
 * Set the RTC freqency to highest possible freq,
 * then using each process open file array set some bits to determine what to
 * divide the base rtc frequency by to get the desired virutalized frequency.
 * Maybe make a linked list in rtc driver that keeps track of each process
 * desired frequency.  rtc open and close add and remove those entries, read
 * blocks based on these entries
 *
 */

/* Some misc. RTC testing functionality */
void test_rtc_enable(void);
void test_rtc_disable(void);
void test_rtc_ticks_incr(void);
uint32_t test_rtc_get_ticks(void);

// END  CP1.4

#endif /* _RTC_H */
