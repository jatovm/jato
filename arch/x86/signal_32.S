.global signal_bh_trampoline
.text

/*
 * signal_bh_trampoline - saves the caller saved registers and calls
 *                        signal bottom half hanlder.
 */
signal_bh_trampoline:
	push %ebp
	mov %esp, %ebp

	/* save caller saved registers */
	pushl %eax
	pushl %ecx
	pushl %edx

	/* push signal bottom half handler's argument - address of
	   faulting instruction. */
	pushl 8(%ebp)

	/* call the signal bottom half handler */
	pushl $.bh_return
	pushl 4(%ebp)
	ret

.bh_return:
	/* cleanup call arguments */
	addl $4,%esp

	/* overwrite return address */
	movl %eax, 8(%ebp)

	/* restore caller saved registers */
	popl %edx
	popl %ecx
	popl %eax

	popl %ebp

	/* Skip bottom half hanlder address pushed by signal_install_bh() */
	addl $4,%esp
	ret
