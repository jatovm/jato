#ifndef __X86_FRAME_H
#define __X86_FRAME_H

#include "jit/stack-slot.h"
#include <stdbool.h>

struct vm_method;
struct expression;
struct compilation_unit;

#define X86_STACK_ALIGN		16

struct native_stack_frame {
	void *prev; /* previous stack frame link */
	unsigned long return_address;
	unsigned long args[0];
} __attribute__((packed));

#ifdef CONFIG_X86_32

#define NR_TRAMPOLINE_LOCALS	0

struct jit_stack_frame {
	void *prev; /* previous stack frame link */
	unsigned long return_address;
	unsigned long args[0];
} __attribute__((packed));

#else

#define NR_TRAMPOLINE_LOCALS	14

struct jit_stack_frame {
	void *prev; /* previous stack frame link */
	unsigned long return_address;
	unsigned long args[0];
} __attribute__((packed));

#endif /* CONFIG_X86_32 */

unsigned long frame_local_offset(struct vm_method *, struct expression *);
unsigned long slot_offset(struct stack_slot *slot);
unsigned long slot_offset_64(struct stack_slot *slot);
unsigned long frame_locals_size(struct stack_frame *frame);
unsigned long cu_frame_locals_offset(struct compilation_unit *cu);

#endif
