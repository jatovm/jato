#include <arch/asm.h>

.global signal_bh_trampoline
.text

/*
 * signal_bh_trampoline - saves the caller saved registers and calls
 *                        signal bottom half hanlder.
 */
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
