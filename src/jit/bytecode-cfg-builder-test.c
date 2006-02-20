/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <system.h>
#include <jam.h>
#include <jit-compiler.h>
#include <libharness.h>
#include <basic-block-assert.h>

/* public String defaultString(String s) { if (s == null) { s = ""; } return s; } */
static unsigned char default_string[9] = {
	/* 0 */ OPC_ALOAD_1,
	/* 1 */ OPC_IFNONNULL, 0x00, 0x07,
	/* 4 */ OPC_LDC, 0x02,
	/* 6 */ OPC_ASTORE_1,
	/* 7 */ OPC_ALOAD_1,
	/* 8 */ OPC_ARETURN,
};

void test_branch_opcode_ends_basic_block(void)
{
	struct basic_block *bb1, *bb2, *bb3;
	struct compilation_unit *cu;
	
	cu = alloc_compilation_unit();
	cu->code = default_string,
	cu->code_len = ARRAY_SIZE(default_string),

	build_cfg(cu);
	assert_int_equals(3, nr_bblocks(cu->entry_bb));

	bb1 = cu->entry_bb;
	bb2 = bb1->next;
	bb3 = bb2->next;

	assert_basic_block(0, 4,  bb2, bb1);
	assert_basic_block(4, 7,  bb3, bb2);
	assert_basic_block(7, 9, NULL, bb3);

	free_compilation_unit(cu);
}

/* public boolean greaterThanZero(int i) { return i > 0; } */ 
static unsigned char greater_than_zero[10] = {
	/* 0 */ OPC_ILOAD_1,
	/* 1 */ OPC_IFLE, 0x00, 0x08,
	/* 4 */ OPC_ICONST_1,
	/* 5 */ OPC_GOTO, 0x00, 0x09,
	/* 8 */ OPC_ICONST_0,
	/* 9 */ OPC_IRETURN,
};

void test_multiple_branches(void)
{
	struct compilation_unit *cu;

	cu = alloc_compilation_unit();
	cu->code = greater_than_zero,
	cu->code_len = ARRAY_SIZE(greater_than_zero),

	build_cfg(cu);
	assert_int_equals(4, nr_bblocks(cu->entry_bb));
}
