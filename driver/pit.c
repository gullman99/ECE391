#include "pit.h"

//https://wiki.osdev.org/Programmable_Interval_Timer


#define CHNL_0_DATA_OUT 		0x40        // channel 0 data port (read/write)
#define ADDR_MODE_OUT   		0x43 		// Mode/Command register (write only, a read is ignored) cmd port 
#define CMD_PIT_OUT_PORT 		0x36 

#define HIGHEST_PIT_RATE 		1193182   	// highest pit rate 
#define FULL_SHIFT				0xFF
#define SHIFT_BY_8				8 	
#define LOWEST_FREQUENCY 		65536		// lowest possible frequency 

 /* NAME: set_pit_rate
	INPUT: rate: takes in the rate 
	OUTPUT: none 
	DISCRIPTION: Responsible for setting the rate 
	Disable interrupts before just in case. */

void set_pit_rate(uint32_t rate){
	
	uint32_t rate_of_pit ;
	
	rate_of_pit = HIGHEST_PIT_RATE / rate ; // remainder acts as divisor 
	
	outb(CMD_PIT_OUT_PORT, ADDR_MODE_OUT ) ; 
	outb(CHNL_0_DATA_OUT ,rate_of_pit & FULL_SHIFT ) ;  //16 bit reload value set low byte of PIT reload/divisor value 
	outb(CHNL_0_DATA_OUT , (rate_of_pit) >> 8  ); 	// set high bits of reload value set high byte of PIT reload/divisor value 
	
	
	
}
/* Initializes PIT to 10ms  */
void pit_init(void){
	
	set_pit_rate((uint32_t)LOWEST_FREQUENCY ) ; // uses set_pit_rate function to initialize PIT to 10ms 
												
												// Maybe want to have a tick counter or set to 0 
	
	}


