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

	/* save FPU registers */
	sub $64, %esp
	movsd %xmm0, (%esp)
	movsd %xmm1, 8(%esp)
	movsd %xmm2, 16(%esp)
	movsd %xmm3, 24(%esp)
	movsd %xmm4, 32(%esp)
	movsd %xmm5, 40(%esp)
	movsd %xmm6, 48(%esp)
	movsd %xmm7, 56(%esp)

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

	/* restore FPU registers */
	movsd (%esp), %xmm0
	movsd 8(%esp), %xmm1
	movsd 16(%esp), %xmm2
	movsd 24(%esp), %xmm3
	movsd 32(%esp), %xmm4
	movsd 40(%esp), %xmm5
	movsd 48(%esp), %xmm6
	movsd 56(%esp), %xmm7
	add $64, %esp

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

	/* save FPU registers */
	sub $128, %rsp
	movsd %xmm0,     (%rsp)
	movsd %xmm1,    8(%rsp)
	movsd %xmm2,   16(%rsp)
	movsd %xmm3,   24(%rsp)
	movsd %xmm4,   32(%rsp)
	movsd %xmm5,   40(%rsp)
	movsd %xmm6,   48(%rsp)
	movsd %xmm7,   56(%rsp)
	movsd %xmm8,   64(%rsp)
	movsd %xmm9,   72(%rsp)
	movsd %xmm10,  80(%rsp)
	movsd %xmm11,  88(%rsp)
	movsd %xmm12,  96(%rsp)
	movsd %xmm13, 104(%rsp)
	movsd %xmm14, 112(%rsp)
	movsd %xmm15, 120(%rsp)

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

	/* restore FPU registers */
	movsd    (%rsp), %xmm0
	movsd   8(%rsp), %xmm1
	movsd  16(%rsp), %xmm2
	movsd  24(%rsp), %xmm3
	movsd  32(%rsp), %xmm4
	movsd  40(%rsp), %xmm5
	movsd  48(%rsp), %xmm6
	movsd  56(%rsp), %xmm7
	movsd  64(%rsp), %xmm8
	movsd  72(%rsp), %xmm9
	movsd  80(%rsp), %xmm10
	movsd  88(%rsp), %xmm11
	movsd  96(%rsp), %xmm12
	movsd 104(%rsp), %xmm13
	movsd 112(%rsp), %xmm14
	movsd 120(%rsp), %xmm15
	add $128, %rsp

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


