#ifndef __JIT_BASIC_BLOCK_ASSERT_H
#define __JIT_BASIC_BLOCK_ASSERT_H

#include <libharness.h>

static void inline
assert_basic_block(struct compilation_unit *parent, unsigned long start,
		   unsigned long end, struct basic_block *bb)
{
	assert_ptr_equals(parent, bb->b_parent);
	assert_int_equals(start, bb->start);
	assert_int_equals(end, bb->end);
}

static inline void
__assert_bb_neighbors(struct basic_block **neigh, int nneigh,
		      struct basic_block **array, unsigned long sz)
{
	int i;

	assert_int_equals(sz, nneigh);

	for (i = 0; i < nneigh; i++)
		assert_ptr_equals(neigh[i], array[i]);
}

static inline void
assert_basic_block_successors(struct basic_block **succ, int nsucc,
			      struct basic_block *bb)
{
	__assert_bb_neighbors(succ, nsucc, bb->successors, bb->nr_successors);
}

static inline void
assert_basic_block_predecessors(struct basic_block **pred, int npred,
				struct basic_block *bb)
{
	__assert_bb_neighbors(pred, npred, bb->predecessors, bb->nr_predecessors);
}

#endif
