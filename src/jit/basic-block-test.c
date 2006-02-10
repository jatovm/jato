/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <basic-block.h>

#include <libharness.h>

void test_split_basic_block(void)
{
	struct basic_block *block = alloc_basic_block(0, 3);
	struct basic_block *bottom = bb_split(block, 2);
	assert_int_equals(0, block->start);
	assert_int_equals(2, block->end);
	assert_int_equals(2, bottom->start);
	assert_int_equals(3, bottom->end);
	free_basic_block(block);
	free_basic_block(bottom);
}
