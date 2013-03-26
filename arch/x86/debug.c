#include "arch/debug.h"

#include "arch/stack-frame.h"

#include <stdlib.h>
#include <stdio.h>

void check_stack_align(void)
{
#ifdef CONFIG_X86_32
	register unsigned long fp __asm__("ebp");
#else
	register unsigned long fp __asm__("rbp");
#endif

	if (fp % X86_STACK_ALIGN != 0) {
		fprintf(stderr, "Error: stack is %lu bytes misaligned.\n", fp % X86_STACK_ALIGN);
		abort();
	}
}
