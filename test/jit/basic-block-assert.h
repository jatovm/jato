#ifndef __JIT_BASIC_BLOCK_ASSERT_H
#define __JIT_BASIC_BLOCK_ASSERT_H

#include <libharness.h>

static void inline assert_basic_block(struct compilation_unit *parent,
				      unsigned long start, unsigned long end,
				      struct basic_block *bb)
{
	assert_ptr_equals(parent, bb->b_parent);
	assert_int_equals(start, bb->start);
	assert_int_equals(end, bb->end);
}

static void inline assert_basic_block_successors(struct basic_block **succ,
						 int nsucc,
						 struct basic_block *bb)
{
	int i;

	assert_int_equals(bb->nr_successors, nsucc);

	for (i = 0; i < nsucc; i++)
		assert_ptr_equals(succ[i], bb->successors[i]);
}

#endif
