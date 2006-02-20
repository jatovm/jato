#ifndef __JIT_BASIC_BLOCK_ASSERT_H
#define __JIT_BASIC_BLOCK_ASSERT_H

#include <libharness.h>

static void inline assert_basic_block(unsigned long start, unsigned long end,
				      struct basic_block *next,
				      struct basic_block *bb)
{
	assert_int_equals(start, bb->start);
	assert_int_equals(end, bb->end);
	assert_ptr_equals(next, bb->next);
}

#endif
