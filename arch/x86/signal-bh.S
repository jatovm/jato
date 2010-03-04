#include <arch/asm.h>

.global signal_bh_trampoline
.text

#ifdef CONFIG_X86_32

/*
 * signal_bh_trampoline - saves the caller saved registers and calls
 *                        signal bottom half hanlder.
 */
.type signal_bh_trampoline, @function
.func signal_bh_trampoline
signal_bh_trampoline:
	push %ebp
	movl %esp, %ebp

	/* save caller saved registers */
	push %eax
	push %ecx
	push %edx

	/* push signal bottom half handler's argument - address of
	   faulting instruction. */
	push 0x04(%ebp)

	/* call the signal bottom half handler */
	push $.bh_return
	push 0x08(%ebp)
	ret
.endfunc

.bh_return:
	/* cleanup call arguments */
	addl $0x04, %esp

	/* overwrite return address */
	movl %eax, 0x08(%ebp)

	/* restore caller saved registers */
	pop %edx
	pop %ecx
	pop %eax

	pop %ebp

	/*
	 * Cleanup bottom half handler address pushed by signal_install_bh().
	 * We do this by popping the return address from stack into
	 * the call argument.
	 */
	addl $0x04, %esp
	ret

#else /* CONFIG_X86_32 */

/*
 * signal_bh_trampoline - saves the caller saved registers and calls
 *                        signal bottom half hanlder.
 */
.type signal_bh_trampoline, @function
.func signal_bh_trampoline
signal_bh_trampoline:
	push %rbp
	movq %rsp, %rbp

	/* save caller saved registers */
	push %rax
	push %rdi
	push %rsi
	push %rdx
	push %rcx
	push %r8
	push %r9

	/* push signal bottom half handler's argument - address of
	   faulting instruction. */
	movq 0x08(%rbp), %rdi

	/* call the signal bottom half handler */
	push $.bh_return
	push 0x10(%rbp)
	ret
.endfunc

.bh_return:
	/* overwrite return address */
	movq %rax, 0x10(%rbp)

	/* restore caller saved registers */
	pop %r9
	pop %r8
	pop %rcx
	pop %rdx
	pop %rsi
	pop %rdi
	pop %rax

	pop %rbp

	/*
	 * Cleanup bottom half handler address pushed by signal_install_bh().
	 * We do this by popping the return address from stack into
	 * the call argument.
	 */
	addq $0x08, %rsp
	ret

#endif /* CONFIG_X86_32 */


