#@author: Michael Gulson
#@net-id: mgulson2
#last edit 2/4/19

.data					# section declaration

        # Useful offset constants for accessing members of a
        # struct mp1_blink_struct structure
        LOCATION   = 0
        ON_CHAR    = 2
        OFF_CHAR   = 3
        ON_LENGTH  = 4
        OFF_LENGTH = 6
        COUNTDOWN  = 8
        STATUS     = 10
        NEXT       = 12

        STRUCT_SIZE = 16

# Pointer to head of list (initialized to NULL)
mp1_list_head:
        .long   0

.text					# section declaration

# Export the function symbol names

.global mp1_rtc_tasklet
.global mp1_ioctl

# void mp1_poke(void);
#
# Interface: Register-based arguments (not C-style)
#    Inputs: %cl  - The byte you wish to write
#            %eax - Offset from the start of video memory that you wish
#                   to write to
#   Outputs: Text-mode video screen is written to at location %eax with
#            the byte in %cl
# Registers: Clobbers EDX

mp1_poke:

	movl    vmem_base_addr(,1),%edx
	movb    %cl,(%edx,%eax,1)
	ret

#void linked_list_traversal();   (to find struct with matching location)
#clobbers edi,ebx
#input: ebx-head of linked list, %cx-location
#return: ebx- struct that matches location (%ebx is null if no matching element found)
linked_list_traversal:
	xorl	%edi, %edi
	movw	LOCATION(%ebx),%di	 #%edi<-user_struct.location
	cmpw	%di, %cx
	je		linked_list_traversal_location_match
	movl	NEXT(%ebx), %ebx
	cmpl	$0, %ebx
	je		linked_list_traversal_return_null
	jmp linked_list_traversal


linked_list_traversal_return_null:
	movl	$0, %ebx
	ret

linked_list_traversal_location_match:
	#ebx-struct that matches location
	ret





# void mp1_rtc_tasklet(unsigned long);
#
# Interface: Argument(arg) on stack, C-style
# Inputs: unsigned long
# Outputs: void
# Registers: Clobbers eax
# Description: updates the state of the game

mp1_rtc_tasklet:
	pushl   %ebp      		# creating new stack frame
 	movl    %esp, %ebp    	# setting new %ebp
 	pushl	%edi
	pushl	%esi
	pushl	%ebx

	##struct.countdown--
	movl 	mp1_list_head, %ebx

linked_list_traversal_loop_rtc_tasklet:
	xorl	%eax, %eax
	movw	COUNTDOWN(%ebx), %ax
	dec		%ax
	movw	%ax, COUNTDOWN(%ebx)

	##check if countdown=0
	cmpw 	$0, %ax
	je		countdown_zero_rtc_tasklet
	####
	movl	NEXT(%ebx), %ebx
	cmpl	$0, %ebx
	je		return_zero
	jmp		linked_list_traversal_loop_rtc_tasklet


countdown_zero_rtc_tasklet:
	#CHECK if STATUS IS ZERO
	xorl	%edi, %edi
	movw	STATUS(%ebx), %di
	cmpw	$0, %di
	je		status_zero_rtc_tasklet

	###STATUS IS 1, TURN OFF
	#####MULTIPLY offset times two
	xorl	%eax, %eax
	xorl 	%ecx, %ecx

	movw 	LOCATION(%ebx),%ax
	cmpl	$1999, %eax 			#check if location is greater than 80*25-1
	ja		return_neg_one
	cmpl	$0,	%eax
	jb		return_neg_one			#check if location is negative

	addl	%eax, %eax				#mult by 2
	movb	OFF_CHAR(%ebx), %cl		#%cl<-struct.offchar

	#POKE
	call 	mp1_poke

	#ON_lENGTH->COUNTDOWN
	xorl	%edi, %edi
	movw 	OFF_LENGTH(%ebx), %di
	movw 	%di, COUNTDOWN(%ebx)

	##change status to off
	xorl	%edi, %edi
	movw	%di, STATUS(%ebx)

	#keep traversing to next element
	movl	NEXT(%ebx), %ebx
	cmpl	$0, %ebx
	je		return_zero
	jmp		linked_list_traversal_loop_rtc_tasklet

status_zero_rtc_tasklet:
	###STATUS IS 0, TURN ON
	#####MULTIPLY offset times two
	xorl	%eax, %eax
	xorl 	%ecx, %ecx
	movw 	LOCATION(%ebx),%ax

	cmpl	$1999, %eax 			#check if location is greater than 80*25-1
	ja		return_neg_one
	cmpl	$0,	%eax
	jb		return_neg_one			#check if location is negative
	addl	%eax, %eax				###multiply by 2

	movb	ON_CHAR(%ebx), %cl

	call 	mp1_poke				#POKE

	#OFF_lENGTH->COUNTDOWN
	xorl	%edi, %edi
	movw 	ON_LENGTH(%ebx), %di
	movw 	%di, COUNTDOWN(%ebx)

	##change status to on
	xorl	%edi, %edi
	inc		%di
	movw	%di, STATUS(%ebx)
	
	movl	NEXT(%ebx), %ebx
	cmpl	$0, %ebx
	je		return_zero
	jmp		linked_list_traversal_loop_rtc_tasklet


# int mp1_ioctl(unsigned long arg, unsigned long cmd);
#
# Interface: arg and cmd on stack, C-style
# Inputs: unsigned long arg, unsigned long cmd
# Outputs: %eax
# Registers: Clobbers eax and ecx
# Description:  "dispatcher" function based on cmd argument

mp1_ioctl:
 	movl    8(%esp), %ecx   			#  %ecx = cmd
	cmpl	$3, %ecx 					#set flags based on (ECX-3)
	ja 		return_neg_one_ioctl		#invalid cmd (cmd>3

####pushl	%ebx### no need to push argument will be on stack
	jmp *jump_table(,%ecx,4)
	ret
	
	
# int mp1_ioctl_add(unsigned long arg);
#
# Interface: arg on stack, C-style
# Inputs: unsigned long arg (pointer to mp1_blink_struct)
# Outputs: %eax
# Registers: Clobbers eax and ecx
# Description:  adds mp1_blink_struct to linked list

mp1_ioctl_add:
	pushl   %ebp      		# creating new stack frame
 	movl    %esp, %ebp    	# setting new %ebp
 	pushl	%edi
	pushl	%esi
	pushl	%ebx

	movl    8(%ebp),  %edi  #  %edi = arg
	cmpl	$0,%edi
	je		return_neg_one

	pushl 	$STRUCT_SIZE		#push 16 stack
	call 	mp1_malloc			#call mp1_malloc
	#eax now has void pointer
	
	cmpl 	$0, %eax
	je   	return_neg_one

	#ANY ERROR CODE FROM HERE ON MUST CALL MP1_free
	addl 	$4, %esp

	xorl	%ebx, %ebx			#%ebx =0
	xchgl	%eax, %ebx			#%eax<->%ebx
								#that way address isn't clobbered
								#with call to mp1_copy_from_user
	#%ebx now has void pointer

	pushl 	$STRUCT_SIZE
	pushl 	%edi			#void * from (user-level pointer)
	pushl	%ebx			#void * to (mallocated memory)
	call 	mp1_copy_from_user
	add		$12, %esp
	cmpl  	$0, %eax
	jne		return_neg_one_free		#after malloc must free memory

	##set countdown=on_length
	movw	ON_LENGTH(%ebx),%di
	movw	%di,COUNTDOWN(%ebx)			#countdown=on_length

	##set status=1
	movw	$1,STATUS(%ebx)

	##insert structure at head of linked list
	xorl	%eax, %eax
	movl	mp1_list_head, %eax
	movl 	%eax, NEXT(%ebx)
	movl 	%ebx, mp1_list_head

	xorl	%eax, %eax
	xorl 	%ecx, %ecx
	movw 	LOCATION(%ebx),%ax

	cmpl	$1999, %eax 		#check if location is greater than 80*25-1
	ja		return_neg_one_free
	cmpl	$0,	%eax
	jb		return_neg_one_free	#check if location is negative

	addl	%eax, %eax			#MULTIPLY offset times two
	movb	ON_CHAR(%ebx), %cl	#struct.ON_CHAR<-%cl
	call 	mp1_poke			#POKE
	jmp     return_zero

	
# int mp1_ioctl_remove(unsigned long arg);
#
# Interface: arg on stack, C-style
# Inputs: unsigned long arg (location)
# Outputs: %eax
# Registers: Clobbers eax and ecx
# Description:  removes mp1_blink_struct to linked list

mp1_ioctl_remove:
	pushl   %ebp      		# creating new stack frame
 	movl    %esp, %ebp    	# setting new %ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx

	movl	mp1_list_head, %ebx		#ebx has mp1_list_head
	movl	8(%ebp),%ecx	#ecx<- arg(location)

	#check if first element needs to be removed
	xorl	%edi, %edi
	movw	LOCATION(%ebx),%di	 #%edi<-user_struct.location
	cmpw	%di, %cx
	jne		linked_list_traversal_loop_remove

	#change head to point to next element
	movl	NEXT(%ebx), %eax
	movl	%eax, mp1_list_head

	#remove 1st element
	pushl	%ebx
	call	mp1_free
	jmp 	return_zero
	
linked_list_traversal_loop_remove:
	xorl	%edi, %edi
	movw	LOCATION(%ebx),%di	 #%edi<-user_struct.location
	cmpw	%di, %cx
	je		location_match_remove
	movl	%ebx, %esi			#esi has previous node
	movl	NEXT(%ebx), %ebx
	cmpl	$0, %ebx
	je		return_neg_one
	jmp linked_list_traversal_loop_remove

location_match_remove:
	###change next location of previous node
	movl	NEXT(%ebx), %eax
	movl	%eax, NEXT(%esi)
	pushl	%ebx
	call	mp1_free
	jmp return_zero


# int mp1_ioctl_find(unsigned long arg);
#
# Interface: arg on stack, C-style
# Inputs: unsigned long arg(pointer to mp1_blink_struct)
# Outputs: %eax
# Registers: Clobbers eax and ecx
# Description:  finds a struct with a matching location and copies the entire struct 
# to user-level

mp1_ioctl_find:
	pushl   %ebp      		# creating new stack frame
 	movl    %esp, %ebp    	# setting new %ebp
	pushl	%edi			#push callee saved registers
	pushl	%esi
	pushl	%ebx

	movl	4(%ebp),%ecx	#ecx<- arg(struct)
	cmpl	$0, %ecx
	je		return_neg_one

	xorw	%ax,%ax
	movw	LOCATION(%ecx),%ax 		#ax has a arg(location)
	
	xorl	%ebx, %ebx				#ebx = 0
	movl 	mp1_list_head, %ebx

linked_list_traversal_loop_find:
	movw	LOCATION(%ebx), %di		#
	cmpw	%di, %ax
	je		location_match_find
	movl	NEXT(%ebx), %ebx
	cmpl	$0, %ebx
	je		return_neg_one
	jmp		linked_list_traversal_loop_find

location_match_find:
	pushl 	STRUCT_SIZE		#copy_to_user(arg, struct_at_location(found in traversal),16)
	pushl 	%ebx			#arguments pushed right to left
	pushl	%ecx
	call 	mp1_copy_to_user
	jmp 	return_zero


# int mp1_ioctl_sync(unsigned long arg);
#
# Interface: arg on stack, C-style
# Inputs: unsigned long arg(two 2 byte ints [int1,int2])
# Outputs: %eax
# Registers: Clobbers eax, edx, ecx
# Description:  searches for 2 structs that match the location values int1 and int2
# syncs the second struct to the same rate as the struct at int1

mp1_ioctl_sync:
	pushl   %ebp      		# creating new stack frame
 	movl    %esp, %ebp    	# setting new %ebp
	pushl	%edi
	pushl	%esi
	pushl	%ebx


	#ebx has mp1_list_head
	movl	mp1_list_head, %ebx

	#arg->%esi  arg=[int1,int2]
	movl	8(%ebp),%esi


	#dx<- arg1[7:0](int2)
	movl	$0x0000FFFF, %eax
	AND		%esi, %eax
	xorl	%edx, %edx
	movw	%ax, %dx

	#cx<- arg1[16:8](int1)
	movl	$0xFFFF0000, %eax
	AND		%esi, %eax
	shr		$16, %eax
	xorl 	%ecx,%ecx
	movw	%ax, %cx

	#clobbers edi,ebx
	#input: ebx-head of linked list, %cx-location
	call linked_list_traversal
	#return: ebx- struct that matches location (%ebx is null if no matching element found)
	cmpl	$0, %ebx
	je 	return_neg_one

	#%ebx->%esi struct1 now saved in esi
	movl	%ebx, %esi

#####reset ebx and save location_1 in edx
	movl	mp1_list_head, %ebx	  #ebx has mp1_list_head

	#search for location 2(%edx->%ecx) or (%ecx=int2)
	xorl	%ecx, %ecx
	movl	%edx, %ecx
	#clobbers edi,ebx
	#input: ebx-head of linked list, %cx-location
	call linked_list_traversal
	#return: ebx- struct that matches location (%ebx is null if no matching element found)
	cmpl	$0, %ebx
	je 	return_neg_one

###change member vars of struct2=struct1
	#esi = pointer to struct1
	#ebx = pointer to struct2

	xorl	%eax, %eax					#struct1.on_length->struct2.on_length
	movw	ON_LENGTH(%esi), %ax
	movw	%ax, ON_LENGTH(%ebx)

	xorl	%eax, %eax
	movw	OFF_LENGTH(%esi), %ax		#struct1.off_length->struct2.off_length
	movw	%ax, OFF_LENGTH(%ebx)

	xorl	%eax, %eax
	movw	COUNTDOWN(%esi), %ax		#struct1.countdown->struct2.countdown
	movw	%ax, COUNTDOWN(%ebx)

	xorl	%eax, %eax
	movw	STATUS(%esi), %ax			#struct1.status->struct2.status
	movw	%ax, STATUS(%ebx)

	###MP1_poke
	xorl	%eax, %eax
	xorl	%ecx, %ecx
	movw 	LOCATION(%esi),%ax

	cmpl	$1999, %eax 		#check if location is greater than 80*25-1
	ja		return_neg_one
	cmpl	$0,	%eax
	jb		return_neg_one		#check if location is negative

	addl	%eax, %eax		#double b/c 2 bytes per chars
	
	xorl	%edi, %edi
	movw	STATUS(%ebx), %di
	cmpw	$0, %di
	je		POKE_OFF
	
	movb	ON_CHAR(%ebx), %cl
	#POKE
	call 	mp1_poke
	jmp		return_zero


POKE_OFF:
	movb	OFF_CHAR(%ebx), %cl
	call 	mp1_poke
	jmp		return_zero


#return negative one without popping off anything off stack
return_neg_one_ioctl	:
	xorl    %eax, %eax    	# eax = 0
	dec 	%eax
	ret

#return negative one popping off callee save registers and ebp
return_neg_one	:
	xorl    %eax, %eax    	# eax = 0
	dec 	%eax
	popl	%ebx
	popl	%esi
	popl	%edi
	leave
	ret

#return zero popping off callee save registers and ebp
return_zero:
	xorl    %eax, %eax    	# eax = 0
	popl	%ebx
	popl	%esi
	popl	%edi
	leave
	ret

#return negative one popping off callee save registers and ebp and freeing memory
return_neg_one_free:
	xorl    %eax, %eax    	# eax = 0
	dec 	%eax
	#push mallocated memory
	pushl 	%ebx
	call 	mp1_free
	popl	%ebx
	popl	%esi
	popl	%edi
	leave
	ret


#jump table for mp1_ioctl
jump_table:
.long mp1_ioctl_add, mp1_ioctl_remove, mp1_ioctl_find, mp1_ioctl_sync

.end
