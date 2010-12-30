/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include "jit/basic-block.h"
#include "jit/compilation-unit.h"
#include <basic-block-assert.h>
#include "jit/statement.h"
#include "vm/vm.h"

#include "arch/instruction.h"

#include <libharness.h>
#include <stddef.h>

static struct cafebabe_method_info method_info;
static struct vm_method method = { .method = &method_info, };

void test_split_with_out_of_range_offset(void)
{
	struct compilation_unit *cu;
	struct basic_block *bb;

	cu = compilation_unit_alloc(&method);
	bb = get_basic_block(cu, 1, 2);

	assert_ptr_equals(NULL, bb_split(bb, 0));
	assert_ptr_equals(NULL, bb_split(bb, 3));

	free_compilation_unit(cu);
}

void test_split_basic_block(void)
{
	struct basic_block *bb, *new_bb;
	struct compilation_unit *cu;

	cu = compilation_unit_alloc(&method);
	bb = get_basic_block(cu, 0, 3);

	new_bb = bb_split(bb, 2);

	assert_basic_block(cu, 0, 2, bb);
	assert_basic_block(cu, 2, 3, new_bb);

	free_compilation_unit(cu);
}

void test_split_basic_block_with_branch(void)
{
	struct basic_block *bb, *new_bb, *target_bb;
	struct compilation_unit *cu;

	cu = compilation_unit_alloc(&method);
	bb = get_basic_block(cu, 0, 4);

	target_bb = bb_split(bb, 3);

	bb->has_branch = true;
	bb_add_successor(bb, target_bb);

	new_bb = bb_split(bb, 2);

	assert_true(new_bb->has_branch);
	assert_false(bb->has_branch);

	assert_basic_block_successors((struct basic_block*[]){         }, 0, bb);
	assert_basic_block_successors((struct basic_block*[]){target_bb}, 1, new_bb);

	free_compilation_unit(cu);
}
