.global unwind
.global exception_check
.text

/*
 * unwind - Performs the following:
 *          1) Resets the stack pointer to point at the end of
 *             caller's stack frame
 *          2) Rethrows the exception in caller's context
 */
.type unwind, @function
.func unwind
/* FIXME: Provide proper prolog and epilog for annotations to work. */
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

	# return address of the method
	pop	%rdx
	decq	%rdx

	movq	%rbp, %rsp
	subq	%rax, %rsp

	/* cu is already in %rdi */
	movq	%rbp, %rsi
	/* native ptr is already in %rdx */
	call	throw_from_jit

	pushq %rax
	ret
.endfunc

/*
 * exception_check - is a part of valgrind workaround for exception guards.
 * it checks whether exception has occurred and if it has control
 * is transfered directly to exception handler (possibly unwind block).
 */
.type exception_check, @function
.func exception_check
/* FIXME: Provide proper prolog and epilog for annotations to work. */
exception_check:
	/* push return address - 1 */
	push	(%rsp)
	decq	(%rsp)

	movq	(%rsp), %rdi	# return address - 1
	call	jit_lookup_cu

	movq	%rax, %rdi	# cu
	movq	%rbp, %rsi	# stack frame
	pop	%rdx		# return address - 1
	call	throw_from_jit_checked

	test	%rax, %rax
	jz	1f
	push	%rax
	ret
1:
	ret
.endfunc
