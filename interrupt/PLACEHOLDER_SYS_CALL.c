#include "syscall.h"

/* PLACEHOLDER DEFINITIONS, REMOVE AS FUNCTIONS ARE IMPLEMENTED.
This is only to stop errors related to undefined references to function
labels in the syscall.S asm linkage. Remove after you;ve defined them
in the appropriate locations. this file should eventually be deleted.*/

/*int32_t vidmap(uint8_t **screen_start) {
    
    
    return 0;
}*/
int32_t set_handler(uint32_t signum, void *handler_address) {return 0;}
int32_t sigreturn(void) {return 0;}
