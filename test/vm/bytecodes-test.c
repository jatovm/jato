/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <vm/bytecodes.h>
#include <vm/byteorder.h>
#include <vm/system.h>
#include <vm/vm.h>

#include <libharness.h>

static void assert_bc_insn_size(unsigned long expected_len, unsigned char opc)
{
	unsigned char code[] = { opc, 0x00, 0x00 };
	assert_int_equals(expected_len, bc_insn_size(code));
}

void test_size_of_bytecode(void)
{
	assert_bc_insn_size(1, OPC_NOP);
	assert_bc_insn_size(3, OPC_IFEQ);
}

static void assert_wide_bc_insn_size(unsigned long expected_len, unsigned char opc)
{
	unsigned char code[] = { OPC_WIDE, opc, 0x00, 0x00, 0x00 };
	assert_int_equals(expected_len, bc_insn_size(code));
}

void test_size_of_wide_bytecode(void)
{
	assert_wide_bc_insn_size(5, OPC_ILOAD);
	assert_wide_bc_insn_size(5, OPC_FLOAD);
	assert_wide_bc_insn_size(5, OPC_ALOAD);
	assert_wide_bc_insn_size(5, OPC_LLOAD);
	assert_wide_bc_insn_size(5, OPC_DLOAD);
	assert_wide_bc_insn_size(5, OPC_ISTORE);
	assert_wide_bc_insn_size(5, OPC_FSTORE);
	assert_wide_bc_insn_size(5, OPC_ASTORE);
	assert_wide_bc_insn_size(5, OPC_LSTORE);
	assert_wide_bc_insn_size(5, OPC_DSTORE);
	assert_wide_bc_insn_size(5, OPC_RET);
	assert_wide_bc_insn_size(6, OPC_IINC);
}

void test_not_branch_opcode(void)
{
	assert_false(bc_is_branch(OPC_NOP));
}

void test_is_branch_opcode(void)
{
	assert_true(bc_is_branch(OPC_IFEQ));
	assert_true(bc_is_branch(OPC_IFNE));
	assert_true(bc_is_branch(OPC_IFLT));
	assert_true(bc_is_branch(OPC_IFGE));
	assert_true(bc_is_branch(OPC_IFGT));
	assert_true(bc_is_branch(OPC_IFLE));
	assert_true(bc_is_branch(OPC_IF_ICMPEQ));
	assert_true(bc_is_branch(OPC_IF_ICMPNE));
	assert_true(bc_is_branch(OPC_IF_ICMPLT));
	assert_true(bc_is_branch(OPC_IF_ICMPGE));
	assert_true(bc_is_branch(OPC_IF_ICMPGT));
	assert_true(bc_is_branch(OPC_IF_ICMPLE));
	assert_true(bc_is_branch(OPC_IF_ACMPEQ));
	assert_true(bc_is_branch(OPC_IF_ACMPNE));
	assert_true(bc_is_branch(OPC_GOTO));
	assert_true(bc_is_branch(OPC_JSR));
	assert_true(bc_is_branch(OPC_TABLESWITCH));
	assert_true(bc_is_branch(OPC_LOOKUPSWITCH));
	assert_true(bc_is_branch(OPC_IFNULL));
	assert_true(bc_is_branch(OPC_IFNONNULL));
	assert_true(bc_is_branch(OPC_GOTO_W));
	assert_true(bc_is_branch(OPC_JSR_W));
}

static void assert_branch_target(uint16_t expected, unsigned char opc)
{
	unsigned char code[] = { opc, (expected & 0xFF00) >> 8, (expected & 0x00FF) };
	assert_int_equals(expected, bc_target_off(code));
}

void test_branch_target(void)
{
	assert_branch_target(0x0001, OPC_IFEQ);
	assert_branch_target(0xFFFF, OPC_IFEQ);
	assert_branch_target(0x1000, OPC_IFNE);
}

static void assert_wide_branch_target(long expected, unsigned char opc)
{
	unsigned char code[] = {
		opc,
		(expected & 0xFF000000UL) >> 24,
		(expected & 0x00FF0000UL) >> 16,
		(expected & 0x0000FF00UL) >> 8,
		 expected & 0x000000FFUL
	};
	assert_int_equals(expected, bc_target_off(code));
}

void test_wide_branch_target(void)
{
	assert_wide_branch_target(0x00000001UL, OPC_GOTO_W);
	assert_wide_branch_target(0xFFFFFFFFUL, OPC_GOTO_W);
	assert_wide_branch_target(0x00FFFF00UL, OPC_GOTO_W);
	assert_wide_branch_target(0x00000001UL, OPC_JSR_W);
}
