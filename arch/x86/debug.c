#include "arch/debug.h"

#include "arch/stack-frame.h"
#include "vm/method.h"
#include "vm/class.h"
#include "jit/args.h"

#include <stdlib.h>
#include <stdio.h>

void check_stack_align(struct vm_method *vmm)
{
#ifdef CONFIG_X86_32
	register unsigned long fp __asm__("ebp");
#else
	register unsigned long fp __asm__("rbp");
#endif
	unsigned long nr_stack_args;

	nr_stack_args = get_stack_args_count(vmm);

	if (fp % X86_STACK_ALIGN != 0) {
		fprintf(stderr,
			"Error: stack is %lu bytes misaligned when entering function:"
			"\n\n  %s.%s%s.\n\n"
			"which has %lu arguments passed on the stack.\n",
			fp % X86_STACK_ALIGN, vmm->class->name, vmm->name, vmm->type, nr_stack_args);
		abort();
	}
}
