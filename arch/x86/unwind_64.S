.global unwind
.text

/*
 * unwind - Performs the following:
 *          1) Resets the stack pointer to point at the end of
 *             caller's stack frame
 *          2) Rethrows the exception in caller's context
 */
unwind:
	popq	%rdx	# return address of the method
	decq	%rdx

	/*
	 * Lookup pointer to compilation unit.
	 */
	push	%rdx
	call	jit_lookup_cu
	movq 	%rax, %rcx
	pop	%rdx

	/*
	 * Restore stack pointer. We need to restore space for locals too.
	 */
	push	%rdx
	push	%rcx
	call	cu_frame_locals_offset
	pop	%rcx
	pop	%rdx

	movq	%rbp, %rsp
	subq	%rax, %rsp

	pushq	%rdx	# native ptr
	pushq	%rbp	# frame
	pushq	%rcx	# cu
	call	throw_exception_from
	addq	$24, %rsp

	pushq %rax
	ret
