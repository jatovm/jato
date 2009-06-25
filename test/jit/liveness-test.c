/*
 * Copyright (C) 2007  Pekka Enberg
 */

#include <jit/compilation-unit.h>
#include <jit/compiler.h>
#include <vm/bitset.h>
#include <vm/class.h>
#include <vm/method.h>
#include <vm/vm.h>
#include <arch/instruction.h>
#include <libharness.h>

struct vm_method method;

static void assert_live_range(struct live_interval *interval, unsigned long expected_start, unsigned long expected_end)
{
	assert_int_equals(expected_start, interval->range.start);
	assert_int_equals(expected_end, interval->range.end);
}

static void assert_uses(struct basic_block *bb, struct var_info *var)
{
	assert_false(test_bit(bb->def_set->bits, var->vreg));
	assert_true(test_bit(bb->use_set->bits, var->vreg));
}

static void assert_defines(struct basic_block *bb, struct var_info *var)
{
	assert_true(test_bit(bb->def_set->bits, var->vreg));
	assert_false(test_bit(bb->use_set->bits, var->vreg));
}

void test_variable_range_limited_to_basic_block(void)
{
	struct compilation_unit *cu;
	struct var_info *r1, *r2;
	struct basic_block *bb;
	struct insn *insn[3];

	cu = compilation_unit_alloc(&method);
	r1 = get_var(cu);
	r2 = get_var(cu);

	bb = get_basic_block(cu, 0, 3);

	insn[0] = imm_insn(INSN_SETL, 0x01, r1);
	bb_add_insn(bb, insn[0]);

	insn[1] = imm_insn(INSN_SETL, 0x02, r2);
	bb_add_insn(bb, insn[1]);

	insn[2] = arithmetic_insn(INSN_ADD, r1, r2, r2);
	bb_add_insn(bb, insn[2]);

	compute_insn_positions(cu);
	analyze_liveness(cu);

	assert_defines(bb, r1);
	assert_defines(bb, r2);

	assert_live_range(r1->interval, 0, 3);
	assert_live_range(r2->interval, 1, 3);

	assert_ptr_equals(insn[0], r1->interval->insn_array[0]);
	assert_ptr_equals(insn[1], r1->interval->insn_array[1]);
	assert_ptr_equals(insn[2], r1->interval->insn_array[2]);

	assert_ptr_equals(insn[1], r2->interval->insn_array[0]);
	assert_ptr_equals(insn[2], r2->interval->insn_array[1]);

	free_compilation_unit(cu);
}

void test_variable_range_spans_two_basic_blocks(void)
{
	struct basic_block *bb1, *bb2;
	struct compilation_unit *cu;
	struct var_info *r1, *r2;
	struct insn *insn[4];

	cu = compilation_unit_alloc(&method);
	r1 = get_var(cu);
	r2 = get_var(cu);

	bb1 = get_basic_block(cu, 0, 2);
	bb2 = get_basic_block(cu, 2, 4);
	bb_add_successor(bb1, bb2);

	insn[2] = imm_insn(INSN_SETL, 0x02, r2);
	bb_add_insn(bb2, insn[2]);

	insn[3] = arithmetic_insn(INSN_ADD, r1, r2, r2);
	bb_add_insn(bb2, insn[3]);

	insn[0] = imm_insn(INSN_SETL, 0x01, r1);
	bb_add_insn(bb1, insn[0]);

	insn[1] = branch_insn(INSN_JMP, bb2);
	bb_add_insn(bb1, insn[1]);

	compute_insn_positions(cu);
	analyze_liveness(cu);

	assert_defines(bb1, r1);
	assert_defines(bb2, r2);
	assert_uses(bb2, r1);

	assert_live_range(r1->interval, 0, 4);
	assert_live_range(r2->interval, 2, 4);

	assert_ptr_equals(insn[0], r1->interval->insn_array[0]);
	assert_ptr_equals(insn[1], r1->interval->insn_array[1]);
	assert_ptr_equals(insn[2], r1->interval->insn_array[2]);
	assert_ptr_equals(insn[3], r1->interval->insn_array[3]);

	assert_ptr_equals(insn[2], r2->interval->insn_array[0]);
	assert_ptr_equals(insn[3], r2->interval->insn_array[1]);

	free_compilation_unit(cu);
}
