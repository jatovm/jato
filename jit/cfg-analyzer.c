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
#include <stdio.h>
	
static void split_after_branches(struct compilation_unit *cu,
				 struct basic_block *entry_bb,
				 unsigned long *branch_targets)
{
	unsigned char *start, *end, *code;
	struct basic_block *bb;

	bb    = entry_bb;
	start = cu->method->jit_code;
	end   = start + cu->method->code_size;
	code  = start;

	while (code != end) {
		unsigned long opc_size;
		
		opc_size = bytecode_size(code);

		if (bytecode_is_branch(*code)) {
			unsigned long br_target;
			unsigned long offset;
			
			offset    = code-start;
			br_target = bytecode_br_target(code) + offset;

			set_bit(branch_targets, br_target);
			bb = bb_split(bb, offset + opc_size);
			list_add_tail(&bb->bb_list_node, &cu->bb_list);
		}
		code += opc_size;
	}
}

static void split_at_branch_targets(struct compilation_unit *cu,
				    unsigned long *branch_targets)
{
	unsigned char *start, *end, *code;

	start = cu->method->jit_code;
	end   = start + cu->method->code_size;
	code  = start;

	while (code != end) {
		unsigned long opc_size;
		unsigned long offset;

		opc_size = bytecode_size(code);
		offset   = code-start;

		if (test_bit(branch_targets, offset)) {
			struct basic_block *bb;
			
			bb = find_bb(cu, offset);
			if (bb->start != offset) {
				bb = bb_split(bb, offset);
				list_add_tail(&bb->bb_list_node, &cu->bb_list);
			}
		}
		code += opc_size;
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

	split_after_branches(cu, entry_bb, branch_targets);
	split_at_branch_targets(cu, branch_targets);

	free(branch_targets);

	return 0;
}
