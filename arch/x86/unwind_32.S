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
	push	%ecx
	push	%edx
	call	get_cu_from_native_addr
	pop	%edx
	pop	%ecx

	/*
	 * Restore stack pointer. We need to restore space for locals too.
	 */
	push	%ecx
	push	%edx

	push	%eax
	call	cu_frame_locals_offset
	addl	$4, %esp

	pop	%edx
	pop	%ecx

	movl	%ebp, %esp
	subl	%eax, %esp

	/*
	 * Lookup pointer to compilation unit.
	 */
	push	%ecx
	push	%edx
	call	get_cu_from_native_addr
	pop	%edx
	pop	%ecx

	/*
	 * Push exception object reference to handler
	 */
	pushl	%ecx

	pushl	%ecx	# exception
	pushl	%edx	# native ptr
	pushl	%ebp	# frame
	pushl	%eax	# cu
	call	throw_exception_from
	popl	%ecx	# cu -> ECX
	addl	$12, %esp

	test	%eax, %eax
	jnz	finish

	/* Jump to exit block. Do not push exception object */
	add	$4, %esp
	pushl	%ecx
	call	cu_exit_bb_native_ptr
	addl	$4, %esp

finish:
	pushl %eax
	ret
