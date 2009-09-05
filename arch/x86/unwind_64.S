.global unwind
.text

/*
 * unwind - Performs the following:
 *          1) Resets the stack pointer to point at the end of
 *             caller's stack frame
 *          2) Rethrows the exception in caller's context
 */
unwind:
	/*
	 * Lookup pointer to compilation unit.
	 */
	movq	(%rsp), %rdi	# return address of the method
	decq	%rdi
	call	jit_lookup_cu

	/*
	 * Restore stack pointer. We need to restore space for locals too.
	 */
	push	%rax		# save cu

	movq 	%rax, %rdi
	movq	0x08(%rsp), %rsi
	call	cu_frame_locals_offset

	pop	%rdi
	pop	%rdx

	movq	%rbp, %rsp
	subq	%rax, %rsp

	/* cu is already in %rdi */
	movq	%rbp, %rsi
	/* native ptr is already in %rdx */
	call	throw_from_jit

	pushq %rax
	ret
