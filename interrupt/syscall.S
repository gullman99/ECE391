.text

.globl handle_syscall

# table of system calls
handle_syscall_table:
.long halt, execute, read, write, open, close, getargs, vidmap, set_handler, sigreturn


# handle_syscall 
# Assembly function to handle syscall interrupts from INT 0x80.
# EAX - Call number, return value is placed here
# EBX - 1st arg
# ECX - 2nd arg
# EDX - 3rd arg
handle_syscall:
	cld
	pushf			# save the flags and registers
	pushl %ebp	
	pushl %edi
	pushl %esi

	pushl %edx		# puhandle_syscall_tablesh arguments to stack
	pushl %ecx
	pushl %ebx

	cmpl $10, %eax	# checks eax holds a valid syscall (1-10)
	jg error
	cmpl $0, %eax
	jle error
	addl $-1, %eax # modify syscall num to map to the jump table entryie

	call *handle_syscall_table(,%eax, 4) 		# jump to the correct syscall

	jmp done

error:
	xorl %eax, %eax			# return -1 on invalid command
	addl $-1, %eax

done:
	popl %ebx				# pops the args
	popl %ecx
	popl %edx

	popl %esi				# restore registers
	popl %edi
	popl %ebp

	popf					# restore flags
	iret
