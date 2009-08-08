#ifndef MMIX_STACK_FRAME_H
#define MMIX_STACK_FRAME_H

#include <stdbool.h>

struct jit_stack_frame {
	void *prev;
	unsigned long return_address;
};

struct native_stack_frame {
	void *prev;
	unsigned long return_address;
};

#endif /* MMIX_STACK_FRAME_H */
