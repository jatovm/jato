/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <basic-block.h>
#include <basic-block-assert.h>
#include <statement.h>

#include <libharness.h>
#include <stddef.h>

void test_basic_block_has_label_stmt(void)
{
	struct basic_block *b = alloc_basic_block(1, 2);
	assert_not_null(b->label_stmt);
	assert_int_equals(STMT_LABEL, b->label_stmt->type);
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
	struct basic_block *b1, *b2, *split;
	
	b1 = alloc_basic_block(0, 3);
	b2 = alloc_basic_block(3, 5);
	b1->next = b2;
	split = bb_split(b1, 2);

	assert_basic_block(0, 2, split, b1);
	assert_basic_block(2, 3, b2, split);
	assert_basic_block(3, 5, NULL, b2);

	free_basic_block(b1);
	free_basic_block(split);
	free_basic_block(b2);
}

void test_find_basic_block(void)
{
	struct basic_block *b1 = alloc_basic_block(0, 3);
	struct basic_block *b2 = alloc_basic_block(3, 5);
	struct basic_block *b3 = alloc_basic_block(5, 6);

	b1->next = b2;
	b2->next = b3;

	assert_ptr_equals(b1, bb_find(b1, 2));
	assert_ptr_equals(b2, bb_find(b1, 3));
	assert_ptr_equals(b3, bb_find(b1, 5));

	free_basic_block(b1);
	free_basic_block(b2);
	free_basic_block(b3);
}

void test_no_basic_block_when_offset_out_of_range(void)
{
	struct basic_block *block = alloc_basic_block(1, 2);
	assert_ptr_equals(NULL, bb_find(block, 0));
	assert_ptr_equals(NULL, bb_find(block, 2));
	free_basic_block(block);
}

void test_nr_bblocks(void)
{
	struct basic_block *entry_bb = alloc_basic_block(0, 3);
	struct basic_block *second_bb = alloc_basic_block(3, 5);

	assert_int_equals(1, nr_bblocks(entry_bb));
	entry_bb->next = second_bb;
	assert_int_equals(2, nr_bblocks(entry_bb));

	free_basic_block(entry_bb);
	free_basic_block(second_bb);
}
