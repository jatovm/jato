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
	call	cu_frame_total_offset
	pop	%ecx
	pop	%edx

	movl	%ebp, %esp
	subl	%eax, %esp

	pushl	%edx	# native ptr
	pushl	%ebp	# frame
	pushl	%ecx	# cu
	call	throw_from_jit
	addl	$12, %esp

	pushl %eax
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
	pushl	(%esp)
	decl	(%esp)

	pushl	%ebp

	pushl	4(%esp)	# return address
	call	jit_lookup_cu
	add	$4, %esp
	push	%eax

	call throw_from_jit_checked
	addl $12, %esp
	test %eax, %eax
	jz 1f
	pushl %eax
	ret
1:
	ret
.endfunc
