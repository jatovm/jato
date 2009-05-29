/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <libharness.h>
#include <jit/compilation-unit.h>
#include <vm/vm.h>

static struct methodblock method = { };

void test_find_basic_block(void)
{
	struct basic_block *b1;
	struct basic_block *b2;
	struct basic_block *b3;
	struct compilation_unit *cu = compilation_unit_alloc(&method);

	b1 = alloc_basic_block(cu, 0, 3);
	b2 = alloc_basic_block(cu, 3, 5);
	b3 = alloc_basic_block(cu, 5, 6);

	list_add_tail(&b1->bb_list_node, &cu->bb_list);
	list_add_tail(&b2->bb_list_node, &cu->bb_list);
	list_add_tail(&b3->bb_list_node, &cu->bb_list);

	assert_ptr_equals(b1, find_bb(cu, 2));
	assert_ptr_equals(b2, find_bb(cu, 3));
	assert_ptr_equals(b3, find_bb(cu, 5));

	free_compilation_unit(cu);
}

void test_no_basic_block_when_offset_out_of_range(void)
{
	struct compilation_unit *cu = compilation_unit_alloc(&method);
	struct basic_block *block = alloc_basic_block(cu, 1, 2);

	list_add_tail(&block->bb_list_node, &cu->bb_list);
	assert_ptr_equals(NULL, find_bb(cu, 0));
	assert_ptr_equals(NULL, find_bb(cu, 2));

	free_compilation_unit(cu);
}

void test_instruction_positions_are_computed_in_basic_block_order(void)
{
	struct compilation_unit *cu = compilation_unit_alloc(&method);
	struct basic_block *b1, *b2;
	struct insn *insns[4];
	unsigned long i;

	for (i = 0; i < ARRAY_SIZE(insns); i++)
		insns[i] = alloc_insn(INSN_ADD);

	b1 = get_basic_block(cu, 0, ARRAY_SIZE(insns)/2);
	for (i = 0 ; i < ARRAY_SIZE(insns) / 2; i++)
		bb_add_insn(b1, insns[i]);

	b2 = get_basic_block(cu, ARRAY_SIZE(insns) / 2, ARRAY_SIZE(insns));
	for (i = ARRAY_SIZE(insns) / 2 ; i < ARRAY_SIZE(insns); i++)
		bb_add_insn(b2, insns[i]);

	compute_insn_positions(cu);

	for (i = 0; i < ARRAY_SIZE(insns); i++)
		assert_int_equals(i, insns[i]->lir_pos);

	free_compilation_unit(cu);
}
