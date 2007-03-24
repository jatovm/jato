/*
 * Bytecode control-flow analysis.
 *
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <jit/jit-compiler.h>

#include <vm/bitset.h>
#include <vm/bytecodes.h>
#include <vm/stream.h>
#include <vm/vm.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

static void update_branch_successors(struct compilation_unit *cu)
{
	struct basic_block *bb;

	for_each_basic_block(bb, &cu->bb_list) {
		struct basic_block *target_bb;

		if (!bb->has_branch)
			continue;

		target_bb = find_bb(cu, bb->br_target_off);
		assert(target_bb != NULL);
		bb_add_successor(bb, target_bb);
	}
}

static void split_at_branch_targets(struct compilation_unit *cu,
				    struct bitset *branch_targets)
{
	unsigned long offset;

	for (offset = 0; offset < cu->method->code_size; offset++) {
		struct basic_block *bb;

		if (!test_bit(branch_targets->bits, offset))
			continue;
			
		bb = find_bb(cu, offset);
		if (bb->start != offset) {
			struct basic_block *new_bb;

			new_bb = bb_split(bb, offset);
			bb_add_successor(bb, new_bb);
			bb = new_bb;
		}
	}
}

static void split_after_branches(struct stream *stream,
				 struct basic_block *entry_bb,
				 struct bitset *branch_targets)
{
	struct basic_block *bb;

	bb = entry_bb;

	for (; stream_has_more(stream); stream_advance(stream)) {
		unsigned long offset, next_insn_off;
		unsigned long br_target_off;
		struct basic_block *new_bb;
		unsigned char *code;

		code = stream->current;

		if (!bc_is_branch(*code))
			continue;

		offset = stream_offset(stream);
		br_target_off = bc_target_off(code) + offset;
		next_insn_off = offset + bc_insn_size(code);

		new_bb = bb_split(bb, next_insn_off);
		if (!bc_is_goto(*code))
			bb_add_successor(bb, new_bb);

		bb->br_target_off = br_target_off;
		bb->has_branch = true;

		set_bit(branch_targets->bits, br_target_off);

		bb = new_bb;
	}
}

static unsigned char *bytecode_next_insn(struct stream *stream)
{
	unsigned long opc_size;
		
	opc_size = bc_insn_size(stream->current);
	assert(opc_size != 0);
	return stream->current + opc_size;
}

static struct stream_operations bytecode_stream_ops = {
	.new_position = bytecode_next_insn,
};

static void bytecode_stream_init(struct stream *stream, struct methodblock *method)
{
	stream_init(stream, method->jit_code, method->code_size,
		    &bytecode_stream_ops);
}

int analyze_control_flow(struct compilation_unit *cu)
{
	struct bitset *branch_targets;
	struct basic_block *entry_bb;
	struct stream stream;

	branch_targets = alloc_bitset(cu->method->code_size);
	if (!branch_targets)
		return -ENOMEM;

	bytecode_stream_init(&stream, cu->method);

	entry_bb = get_basic_block(cu, 0, cu->method->code_size);
	split_after_branches(&stream, entry_bb, branch_targets);
	split_at_branch_targets(cu, branch_targets);
	update_branch_successors(cu);

	free(branch_targets);

	return 0;
}
