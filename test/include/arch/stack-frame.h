#ifndef MMIX_STACK_FRAME_H
#define MMIX_STACK_FRAME_H

#include <stdbool.h>

struct jit_stack_frame {
	unsigned long return_address;
};

bool is_jit_method(unsigned long eip);

#endif /* MMIX_STACK_FRAME_H */
