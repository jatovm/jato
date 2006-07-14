/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for building a control-flow graph of a bytecode
 * stream.
 */

#include <jit/jit-compiler.h>
#include <bytecodes.h>

#include <vm/bitmap.h>
#include <vm/vm.h>

#include <errno.h>
#include <stdlib.h>
	
static void bb_end_after_branch(struct compilation_unit *cu,
				struct basic_block *entry_bb,
				unsigned long *branch_targets)
{
	struct basic_block *bb;
	unsigned long prev, offset = 0;
	unsigned char *code = cu->method->jit_code;

	bb = entry_bb;

	while (offset < cu->method->code_size) {
		prev = offset;
		offset += bytecode_size(code + offset);

		if (bytecode_is_branch(code[prev])) {
			set_bit(branch_targets, bytecode_br_target(code + prev));
			bb = bb_split(bb, offset);
			list_add_tail(&bb->bb_list_node, &cu->bb_list);
		}
	}
}

static void bb_start_at_branch_target(struct compilation_unit *cu,
				      unsigned long *branch_targets)
{
	unsigned long offset = 0;

	while (offset < cu->method->code_size) {
		if (test_bit(branch_targets, offset)) {
			struct basic_block *bb;
			
			bb = find_bb(cu, offset);

			if (bb->start != offset) {
				bb = bb_split(bb, offset);
				list_add_tail(&bb->bb_list_node, &cu->bb_list);
			}
		}
		offset += bytecode_size(cu->method->jit_code + offset);
	}
}

int build_cfg(struct compilation_unit *cu)
{
	unsigned long *branch_targets;
	struct basic_block *entry_bb;

	branch_targets = alloc_bitmap(cu->method->code_size);
	if (!branch_targets)
		return -ENOMEM;

	entry_bb = alloc_basic_block(cu, 0, cu->method->code_size);
	list_add_tail(&entry_bb->bb_list_node, &cu->bb_list);
	bb_end_after_branch(cu, entry_bb, branch_targets);
	bb_start_at_branch_target(cu, branch_targets);

	free(branch_targets);

	return 0;
}
