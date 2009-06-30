#include <arch/asm.h>

.global signal_bh_trampoline
.text

/*
 * signal_bh_trampoline - saves the caller saved registers and calls
 *                        signal bottom half hanlder.
 */
signal_bh_trampoline:
	push %xbp
	mov %xsp, %xbp

	/* save caller saved registers */
	push %xax
	push %xcx
	push %xdx
#ifdef CONFIG_X86_64
	pushq %r8
	pushq %r9
	pushq %r10
	pushq %r11
#endif

	/* push signal bottom half handler's argument - address of
	   faulting instruction. */
	push (2 * PTR_SIZE)(%xbp)

	/* call the signal bottom half handler */
	push $.bh_return
	push PTR_SIZE(%xbp)
	ret

.bh_return:
	/* cleanup call arguments */
	add $PTR_SIZE, %xsp

	/* overwrite return address */
	mov %xax, (2 * PTR_SIZE)(%xbp)

	/* restore caller saved registers */
#ifdef CONFIG_X86_64
	popq %r11
	popq %r10
	popq %r9
	popq %r8
#endif
	pop %xdx
	pop %xcx
	pop %xax

	pop %xbp

	/* Skip bottom half hanlder address pushed by signal_install_bh() */
	add $PTR_SIZE, %xsp
	ret
