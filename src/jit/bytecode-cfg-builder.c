/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <jam.h>
#include <jit-compiler.h>
#include <bytecodes.h>
#include <bitmap.h>
	
static void split_after_branch(struct compilation_unit *cu,
			       unsigned long *branch_targets)
{
	struct basic_block *bb;
	unsigned long prev, offset = 0;

	bb = cu->entry_bb;

	while (offset < cu->code_len) {
		prev = offset;
		offset += bytecode_size(cu->code + offset);

		if (bytecode_is_branch(cu->code[prev])) {
			set_bit(branch_targets, bytecode_br_target(&cu->code[prev]));
			bb_split(bb, offset);
		}
	}
}

static void split_branch_targets(struct compilation_unit *cu,
				 unsigned long *branch_targets)
{
	unsigned long offset = 0;

	while (offset < cu->code_len) {
		if (test_bit(branch_targets, offset)) {
			struct basic_block *bb = bb_find(cu->entry_bb, offset);
			bb_split(bb, offset);
		}
		offset += bytecode_size(cu->code + offset);
	}
}

void build_cfg(struct compilation_unit *cu)
{
	unsigned long *branch_targets;

	branch_targets = alloc_bitmap(cu->code_len);

	cu->entry_bb = alloc_basic_block(0, cu->code_len);
	split_after_branch(cu, branch_targets);
	split_branch_targets(cu, branch_targets);
}
