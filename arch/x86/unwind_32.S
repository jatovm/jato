.global unwind
.text

/*
 * unwind - Performs the following:
 *          1) Resets the stack pointer to point at the end of
 *             caller's stack frame
 *          2) Rethrows the exception in caller's context
 */
unwind:
	popl	%edx	# return address of the method
	decl	%edx

	/*
	 * Lookup pointer to compilation unit.
	 */
	push	%edx
	call	jit_lookup_cu
	movl 	%eax, %ecx
	pop	%edx

	/*
	 * Restore stack pointer. We need to restore space for locals too.
	 */
	push	%edx
	push	%ecx
	call	cu_frame_locals_offset
	pop	%ecx
	pop	%edx

	movl	%ebp, %esp
	subl	%eax, %esp

	pushl	%edx	# native ptr
	pushl	%ebp	# frame
	pushl	%ecx	# cu
	call	throw_exception_from
	addl	$12, %esp

	pushl %eax
	ret
