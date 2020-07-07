.text

.globl keyboard_isr, rtc_isr, timer_isr, switch_process_debug

.align 4

keyboard_isr:
    cli
    pushfl
    pushal
    call handle_keyboard
    popal
    popfl
    sti
    iret

rtc_isr:
    cli
    pushfl
    pushal
    call handle_rtc
    popal
    popfl
    sti
    iret

timer_isr:
    cli
    pushfl
    pushal
    call handle_timer
    popal
    popfl
    sti
    iret

# This is not an actual ISR, used when manually controlling the switching of processes
switch_process_debug:
    cli
    pushfl
    pushal
    call switch_to_process
    popal
    popfl
    sti
    ret
