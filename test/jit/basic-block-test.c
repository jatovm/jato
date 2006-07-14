/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <jit/basic-block.h>
#include <basic-block-assert.h>
#include <jit/instruction.h>
#include <jit/statement.h>

#include <libharness.h>
#include <stddef.h>

void test_basic_block_has_label_stmt(void)
{
	struct basic_block *b = alloc_basic_block(1, 2);
	assert_not_null(b->label_stmt);
	assert_int_equals(STMT_LABEL, stmt_type(b->label_stmt));
	free_basic_block(b);
}

void test_split_with_out_of_range_offset(void)
{
	struct basic_block *block = alloc_basic_block(1, 2);
	assert_ptr_equals(NULL, bb_split(block, 0));
	assert_ptr_equals(NULL, bb_split(block, 2));
	free_basic_block(block);
}

void test_split_basic_block(void)
{
	struct basic_block *block, *split;
	
	block = alloc_basic_block(0, 3);
	split = bb_split(block, 2);

	assert_basic_block(0, 2, block);
	assert_basic_block(2, 3, split);

	free_basic_block(block);
	free_basic_block(split);
}
