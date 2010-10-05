.global jni_trampoline
jni_trampoline:
	call	vm_enter_jni
	addl	$0xc, %esp		# cleanup args

	test	%eax, %eax
	jne	error

	popl	%eax			# target
	addl	$0x4, %esp		# return address

	call	*%eax
	subl	$0x4, %esp		# new return address

	pushl	%eax			# save return value
	call	vm_leave_jni
	movl	%eax, 0x4(%esp)		# override return address
	popl	%eax			# restore return value

	ret
error:
	addl	$0x4, %esp		# target
	ret
