/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <system.h>
#include <jam.h>
#include <jit-compiler.h>
#include <libharness.h>
#include <basic-block-assert.h>

/*
    public String defaultString(String s) {
        if (s == null)
            s = "";
        return s;
    }
 */
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
	struct basic_block *bb1, *bb2;
	struct compilation_unit cu = {
		.code = default_string,
		.code_len = ARRAY_SIZE(default_string),
	};
	build_cfg(&cu);
	assert_int_equals(2, nr_bblocks(cu.entry_bb));

	bb1 = cu.entry_bb;
	bb2 = bb1->next;

	assert_basic_block(0, 4, bb2, bb1);
	assert_basic_block(4, 9, NULL, bb2);
}
