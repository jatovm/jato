#ifndef __PPC_STACK_FRAME_H
#define __PPC_STACK_FRAME_H

#include "jit/stack-slot.h"

struct native_stack_frame {
	void			*prev;		/* previous stack frame link */
	unsigned long 		return_address;
	unsigned long		args[0];
} __attribute__((packed));

struct jit_stack_frame {
	void			*prev;		/* previous stack frame link */
	unsigned long 		return_address;
	unsigned long		args[0];
};

static inline unsigned long frame_locals_size(struct stack_frame *frame)
{
	return 0;
}

#endif /* __PPC_STACK_FRAME_H */
