#ifndef _PIT_H
#define _PIT_H
#include "../lib.h"
#include "../types.h"
#include "../x86_desc.h"


void set_pit_rate(uint32_t rate); 
void pit_init(void); 

#endif /* _PIT_H */
