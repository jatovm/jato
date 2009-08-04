/*
 * Copyright (C) 2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include "jit/compilation-unit.h"
#include "jit/compiler.h"
#include "jit/vars.h"

#include "arch/instruction.h"

#include "test/vars.h"
#include <bc-test-utils.h>
#include <libharness.h>
#include <stdlib.h>

#define VREG_OFFSET (sizeof(unsigned long) - 1)

DECLARE_STATIC_VREG(r0, VREG_OFFSET + 0);
DECLARE_STATIC_VREG(r1, VREG_OFFSET + 1);
DECLARE_STATIC_VREG(r2, VREG_OFFSET + 2);

#define NR_VREGS VREG_OFFSET + 3

DECLARE_STATIC_REG(eax, REG_EAX);
DECLARE_STATIC_REG(ecx, REG_ECX);
DECLARE_STATIC_REG(edx, REG_EDX);

static void assert_use_mask(int r0_set, int r1_set, int r2_set, struct insn *insn)
{
	assert_int_equals(r0_set, insn_uses(insn, &r0));
	assert_int_equals(r1_set, insn_uses(insn, &r1));
	assert_int_equals(r2_set, insn_uses(insn, &r2));
}

static void assert_def_mask(int r0_set, int r1_set, int r2_set, struct insn *insn)
{
	assert_int_equals(r0_set, insn_defs(insn, &r0));
	assert_int_equals(r1_set, insn_defs(insn, &r1));
	assert_int_equals(r2_set, insn_defs(insn, &r2));
}

static void assert_def_fixed_mask(int a_set, int c_set, int d_set, struct insn *insn)
{
	assert_int_equals(a_set, insn_defs(insn, &eax));
	assert_int_equals(c_set, insn_defs(insn, &ecx));
	assert_int_equals(d_set, insn_defs(insn, &edx));
}

static void assert_does_not_define_or_use_anything(struct insn *insn)
{
	assert_use_mask(0, 0, 0, insn);
	assert_def_mask(0, 0, 0, insn);

	free_insn(insn);
}

static void assert_uses_r0(struct insn *insn)
{
	assert_use_mask(1, 0, 0, insn);
	assert_def_mask(0, 0, 0, insn);

	free_insn(insn);
}

static void assert_uses_r0_defines_r0(struct insn *insn)
{
	assert_use_mask(1, 0, 0, insn);
	assert_def_mask(1, 0, 0, insn);

	free_insn(insn);
}

static void assert_uses_r0_r1(struct insn *insn)
{
	assert_use_mask(1, 1, 0, insn);
	assert_def_mask(0, 0, 0, insn);

	free_insn(insn);
}

static void assert_uses_r0_r1_and_r2(struct insn *insn)
{
	assert_use_mask(1, 1, 1, insn);
	assert_def_mask(0, 0, 0, insn);

	free_insn(insn);
}

static void assert_uses_r0_defines_r1(struct insn *insn)
{
	assert_use_mask(1, 0, 0, insn);
	assert_def_mask(0, 1, 0, insn);

	free_insn(insn);
}

static void assert_uses_r0_defines_r0_and_r1(struct insn *insn)
{
	assert_use_mask(1, 0, 0, insn);
	assert_def_mask(1, 1, 0, insn);

	free_insn(insn);
}

static void assert_uses_r0_r1_defines_r1(struct insn *insn)
{
	assert_use_mask(1, 1, 0, insn);
	assert_def_mask(0, 1, 0, insn);

	free_insn(insn);
}

static void assert_uses_r0_and_r1_defines_r2(struct insn *insn)
{
	assert_use_mask(1, 1, 0, insn);
	assert_def_mask(0, 0, 1, insn);

	free_insn(insn);
}

static void assert_defines_r0(struct insn *insn)
{
	assert_use_mask(0, 0, 0, insn);
	assert_def_mask(1, 0, 0, insn);

	free_insn(insn);
}

static void assert_defines_caller_saved(struct insn *insn)
{
	assert_def_fixed_mask(1, 1, 1, insn);

	free_insn(insn);
}

void test_memlocal_reg_defines_target(void)
{
	struct stack_slot slot;

	assert_defines_r0(memlocal_reg_insn(INSN_MOV_MEMLOCAL_REG, &slot, &r0));
}

void test_membase_reg_uses_source_defines_target(void)
{
	assert_uses_r0_r1_defines_r1(membase_reg_insn(INSN_ADD_MEMBASE_REG, &r0, 0, &r1));
	assert_uses_r0_r1_defines_r1(membase_reg_insn(INSN_AND_MEMBASE_REG, &r0, 0, &r1));
	assert_uses_r0_defines_r1(membase_reg_insn(INSN_CMP_MEMBASE_REG, &r0, 0, &r1));
	assert_uses_r0_r1_defines_r1(membase_reg_insn(INSN_DIV_MEMBASE_REG, &r0, 0, &r1));
	assert_uses_r0_defines_r1(membase_reg_insn(INSN_MOV_MEMBASE_REG, &r0, 0, &r1));
	assert_uses_r0_defines_r1(membase_reg_insn(INSN_MUL_MEMBASE_EAX, &r0, 0, &r1));
	assert_uses_r0_r1_defines_r1(membase_reg_insn(INSN_OR_MEMBASE_REG, &r0, 0, &r1));
	assert_uses_r0_defines_r1(membase_reg_insn(INSN_SUB_MEMBASE_REG, &r0, 0, &r1));
	assert_uses_r0_defines_r1(membase_reg_insn(INSN_SBB_MEMBASE_REG, &r0, 0, &r1));
	assert_uses_r0_defines_r1(membase_reg_insn(INSN_XOR_MEMBASE_REG, &r0, 0, &r1));
}

void test_memindex_reg_uses_source_defines_target(void)
{
	assert_uses_r0_and_r1_defines_r2(memindex_reg_insn(INSN_MOV_MEMINDEX_REG, &r0, &r1, 0, &r2));
}

void test_reg_membase_uses_source_and_target(void)
{
	struct stack_slot slot;

	assert_uses_r0(reg_memlocal_insn(INSN_MOV_REG_MEMLOCAL, &r0, &slot));
}

void test_reg_memindex_uses_source_and_target(void)
{
	assert_uses_r0_r1_and_r2(reg_memindex_insn(INSN_MOV_REG_MEMINDEX, &r0, &r1, &r2, 0));
}

void test_call_reg_uses_operand(void)
{
	assert_uses_r0(reg_insn(INSN_CALL_REG, &r0));
}

void test_call_defines_caller_saved(void)
{
	assert_defines_caller_saved(reg_insn(INSN_CALL_REG, &r0));
	assert_defines_caller_saved(rel_insn(INSN_CALL_REL, 0x12345678));
}

void test_cltd_defines_edx_and_eax_and_uses_eax(void)
{
	assert_uses_r0_defines_r0_and_r1(reg_reg_insn(INSN_CLTD_REG_REG, &r0, &r1));
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
	assert_uses_r0_r1_defines_r1(reg_reg_insn(INSN_SAR_REG_REG, &r0, &r1));
	assert_uses_r0_defines_r1(reg_reg_insn(INSN_SHL_REG_REG, &r0, &r1));
	assert_uses_r0_defines_r1(reg_reg_insn(INSN_SHR_REG_REG, &r0, &r1));
}

void test_imm_reg_defines_target(void)
{
	assert_uses_r0_defines_r0(imm_reg_insn(INSN_ADD_IMM_REG, 0xdeadbeef, &r0));
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

void test_branch_does_not_define_or_use_anything(void)
{
	struct basic_block bb;

	assert_does_not_define_or_use_anything(branch_insn(INSN_JE_BRANCH, &bb));
	assert_does_not_define_or_use_anything(branch_insn(INSN_JGE_BRANCH, &bb));
	assert_does_not_define_or_use_anything(branch_insn(INSN_JG_BRANCH, &bb));
	assert_does_not_define_or_use_anything(branch_insn(INSN_JLE_BRANCH, &bb));
	assert_does_not_define_or_use_anything(branch_insn(INSN_JL_BRANCH, &bb));
	assert_does_not_define_or_use_anything(branch_insn(INSN_JMP_BRANCH, &bb));
	assert_does_not_define_or_use_anything(branch_insn(INSN_JNE_BRANCH, &bb));
}

void test_mul_defines_eax_edx(void)
{
	assert_def_fixed_mask(1, 0, 1, reg_reg_insn(INSN_MUL_REG_EAX, &r2, &r2));
}
