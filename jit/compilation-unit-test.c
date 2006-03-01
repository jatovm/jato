/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <libharness.h>
#include <compilation-unit.h>

void test_find_basic_block(void)
{
	struct basic_block *b1 = alloc_basic_block(0, 3);
	struct basic_block *b2 = alloc_basic_block(3, 5);
	struct basic_block *b3 = alloc_basic_block(5, 6);
	struct compilation_unit *cu = alloc_compilation_unit(NULL, 0, b1);

	list_add_tail(&b2->bb_list_node, &cu->bb_list);
	list_add_tail(&b3->bb_list_node, &cu->bb_list);

	assert_ptr_equals(b1, find_bb(cu, 2));
	assert_ptr_equals(b2, find_bb(cu, 3));
	assert_ptr_equals(b3, find_bb(cu, 5));

	free_compilation_unit(cu);
}

void test_no_basic_block_when_offset_out_of_range(void)
{
	struct basic_block *block = alloc_basic_block(1, 2);
	struct compilation_unit *cu = alloc_compilation_unit(NULL, 0, block);

	assert_ptr_equals(NULL, find_bb(cu, 0));
	assert_ptr_equals(NULL, find_bb(cu, 2));

	free_compilation_unit(cu);
}
