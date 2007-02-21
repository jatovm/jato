/*
 * Copyright (C) 2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <jit/compilation-unit.h>
#include <jit/instruction.h>
#include <jit/jit-compiler.h>

#include <libharness.h>
#include <bc-test-utils.h>

static struct var_info r0 = { .vreg = 0 };
static struct var_info r1 = { .vreg = 1 };
static struct var_info r2 = { .vreg = 2 };

static void assert_does_not_define_or_use_anything(struct insn *insn)
{
	assert_int_equals(0x00, insn_use_mask(insn));
	assert_int_equals(0x00, insn_def_mask(insn));

	free_insn(insn);
}

static void assert_uses_r0(struct insn *insn)
{
	assert_int_equals(0x01, insn_use_mask(insn));
	assert_int_equals(0x00, insn_def_mask(insn));

	free_insn(insn);
}

static void assert_uses_r0_defines_r0(struct insn *insn)
{
	assert_int_equals(0x01, insn_use_mask(insn));
	assert_int_equals(0x01, insn_def_mask(insn));

	free_insn(insn);
}

static void assert_uses_r0_and_r1(struct insn *insn)
{
	assert_int_equals(0x01 | 0x02, insn_use_mask(insn));
	assert_int_equals(0x00, insn_def_mask(insn));

	free_insn(insn);
}

static void assert_uses_r0_r1_and_r2(struct insn *insn)
{
	assert_int_equals(0x01 | 0x02 | 0x04, insn_use_mask(insn));
	assert_int_equals(0x00, insn_def_mask(insn));

	free_insn(insn);
}

static void assert_uses_r0_defines_r1(struct insn *insn)
{
	assert_int_equals(0x01, insn_use_mask(insn));
	assert_int_equals(0x02, insn_def_mask(insn));

	free_insn(insn);
}

static void assert_uses_r0_and_r1_defines_r2(struct insn *insn)
{
	assert_int_equals(0x01 | 0x02, insn_use_mask(insn));
	assert_int_equals(0x04, insn_def_mask(insn));

	free_insn(insn);
}

static void assert_defines_r0(struct insn *insn)
{
	assert_int_equals(0x00, insn_use_mask(insn));
	assert_int_equals(0x01, insn_def_mask(insn));

	free_insn(insn);
}

void test_membase_reg_uses_source_defines_target(void)
{
	assert_uses_r0_defines_r1(membase_reg_insn(INSN_ADD_MEMBASE_REG, &r0, 0, &r1));
	assert_uses_r0_defines_r1(membase_reg_insn(INSN_AND_MEMBASE_REG, &r0, 0, &r1));
	assert_uses_r0_defines_r1(membase_reg_insn(INSN_CMP_MEMBASE_REG, &r0, 0, &r1));
	assert_uses_r0_defines_r1(membase_reg_insn(INSN_DIV_MEMBASE_REG, &r0, 0, &r1));
	assert_uses_r0_defines_r1(membase_reg_insn(INSN_MOV_MEMBASE_REG, &r0, 0, &r1));
	assert_uses_r0_defines_r1(membase_reg_insn(INSN_MUL_MEMBASE_REG, &r0, 0, &r1));
	assert_uses_r0_defines_r1(membase_reg_insn(INSN_OR_MEMBASE_REG, &r0, 0, &r1));
	assert_uses_r0_defines_r1(membase_reg_insn(INSN_SUB_MEMBASE_REG, &r0, 0, &r1));
	assert_uses_r0_defines_r1(membase_reg_insn(INSN_XOR_MEMBASE_REG, &r0, 0, &r1));
}

void test_memindex_reg_uses_source_defines_target(void)
{
	assert_uses_r0_and_r1_defines_r2(memindex_reg_insn(INSN_MOV_MEMINDEX_REG, &r0, &r1, 0, &r2));
}

void test_reg_membase_uses_source_and_target(void)
{
	assert_uses_r0_and_r1(reg_membase_insn(INSN_MOV_REG_MEMBASE, &r0, &r1, 0));
}

void test_reg_memindex_uses_source_and_target(void)
{
	assert_uses_r0_r1_and_r2(reg_memindex_insn(INSN_MOV_REG_MEMINDEX, &r0, &r1, &r2, 0));
}

void test_call_reg_uses_operand(void)
{
	assert_uses_r0(reg_insn(INSN_CALL_REG, &r0));
}

void test_cltd_defines_edx_and_eax_and_uses_eax(void)
{
	// FIXME: INSN_CLTD
}

void test_neg_reg_uses_and_defines_operand(void)
{
	assert_uses_r0_defines_r0(reg_insn(INSN_NEG_REG, &r0));
}

void test_push_reg_uses_operand(void)
{
	assert_uses_r0(reg_insn(INSN_PUSH_REG, &r0));
}

void test_reg_reg_uses_source_defines_target(void)
{
	assert_uses_r0_defines_r1(reg_reg_insn(INSN_MOV_REG_REG, &r0, &r1));
	assert_uses_r0_defines_r1(reg_reg_insn(INSN_SAR_REG_REG, &r0, &r1));
	assert_uses_r0_defines_r1(reg_reg_insn(INSN_SHL_REG_REG, &r0, &r1));
	assert_uses_r0_defines_r1(reg_reg_insn(INSN_SHR_REG_REG, &r0, &r1));
}

void test_imm_reg_defines_target(void)
{
	assert_defines_r0(imm_reg_insn(INSN_ADD_IMM_REG, 0xdeadbeef, &r0));
	assert_uses_r0(imm_reg_insn(INSN_CMP_IMM_REG, 0xdeadbeef, &r0));
	assert_defines_r0(imm_reg_insn(INSN_MOV_IMM_REG, 0xdeadbeef, &r0));
}

void test_imm_membase_uses_target(void)
{
	assert_uses_r0(imm_membase_insn(INSN_MOV_IMM_MEMBASE, 0xdeadbeef, &r0, 0));
}

void test_imm_does_not_define_or_use_anything(void)
{
	assert_does_not_define_or_use_anything(imm_insn(INSN_PUSH_IMM, 0xdeadbeef));
}

void test_rel_does_not_define_or_use_anything(void)
{
	assert_does_not_define_or_use_anything(rel_insn(INSN_CALL_REL, 0xcafebabe));
}

void test_branch_does_not_define_or_use_anything(void)
{
	struct basic_block bb;

	assert_does_not_define_or_use_anything(branch_insn(INSN_JE_BRANCH, &bb));
	assert_does_not_define_or_use_anything(branch_insn(INSN_JNE_BRANCH, &bb));
	assert_does_not_define_or_use_anything(branch_insn(INSN_JMP_BRANCH, &bb));
}
