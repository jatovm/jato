/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <jam.h>
#include <libharness.h>
#include <bytecodes.h>

void test_not_branch_opcode(void)
{
	assert_false(bytecode_is_branch(OPC_NOP));
}

void test_is_branch_opcode(void)
{
	assert_true(bytecode_is_branch(OPC_IFEQ));
	assert_true(bytecode_is_branch(OPC_IFNE));
	assert_true(bytecode_is_branch(OPC_IFLT));
	assert_true(bytecode_is_branch(OPC_IFGE));
	assert_true(bytecode_is_branch(OPC_IFGT));
	assert_true(bytecode_is_branch(OPC_IFLE));
	assert_true(bytecode_is_branch(OPC_IF_ICMPEQ));
	assert_true(bytecode_is_branch(OPC_IF_ICMPNE));
	assert_true(bytecode_is_branch(OPC_IF_ICMPLT));
	assert_true(bytecode_is_branch(OPC_IF_ICMPGE));
	assert_true(bytecode_is_branch(OPC_IF_ICMPGT));
	assert_true(bytecode_is_branch(OPC_IF_ICMPLE));
	assert_true(bytecode_is_branch(OPC_IF_ACMPEQ));
	assert_true(bytecode_is_branch(OPC_IF_ACMPNE));
	assert_true(bytecode_is_branch(OPC_GOTO));
	assert_true(bytecode_is_branch(OPC_JSR));
	assert_true(bytecode_is_branch(OPC_TABLESWITCH));
	assert_true(bytecode_is_branch(OPC_LOOKUPSWITCH));
	assert_true(bytecode_is_branch(OPC_IFNULL));
	assert_true(bytecode_is_branch(OPC_IFNONNULL));
	assert_true(bytecode_is_branch(OPC_GOTO_W));
	assert_true(bytecode_is_branch(OPC_JSR_W));
}
