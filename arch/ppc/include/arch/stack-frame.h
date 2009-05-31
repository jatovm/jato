#ifndef __PPC_STACK_FRAME_H
#define __PPC_STACK_FRAME_H

#include <jit/stack-slot.h>

struct jit_stack_frame {
	unsigned long	return_address;
};

static inline unsigned long frame_locals_size(struct stack_frame *frame)
{
	return 0;
}

#endif /* __PPC_STACK_FRAME_H */
