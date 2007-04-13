/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <jit/basic-block.h>
#include <jit/compilation-unit.h>
#include <basic-block-assert.h>
#include <jit/statement.h>

#include <arch/instruction.h>

#include <libharness.h>
#include <stddef.h>

void test_split_with_out_of_range_offset(void)
{
	struct compilation_unit *cu;
	struct basic_block *bb;

	cu = alloc_compilation_unit(NULL);
	bb = get_basic_block(cu, 1, 2);

	assert_ptr_equals(NULL, bb_split(bb, 0));
	assert_ptr_equals(NULL, bb_split(bb, 2));

	free_compilation_unit(cu);
}

void test_split_basic_block(void)
{
	struct basic_block *bb, *new_bb;
	struct compilation_unit *cu;

	cu = alloc_compilation_unit(NULL);
	bb = get_basic_block(cu, 0, 3);

	new_bb = bb_split(bb, 2);

	assert_basic_block(cu, 0, 2, bb);
	assert_basic_block(cu, 2, 3, new_bb);

	free_compilation_unit(cu);
}
