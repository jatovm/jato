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
		bb->successors[1] = target_bb;
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

static struct basic_block *do_split(struct compilation_unit *cu,
				    struct basic_block *bb,
				    struct stream *stream,
				    struct bitset *branch_targets)
{
	unsigned long current_offset;
	unsigned long br_target_off;
	struct basic_block *new_bb;

	current_offset = stream_offset(stream);
	br_target_off = bc_target_off(stream->current) + current_offset;

	set_bit(branch_targets->bits, br_target_off);
	bb->br_target_off = br_target_off;
	bb->has_branch = true;
	new_bb = bb_split(bb, current_offset + bc_insn_size(stream->current));

	if (!bc_is_goto(*stream->current))
		bb->successors[0] = new_bb;

	return new_bb;
}

static void split_after_branches(struct compilation_unit *cu,
				 struct basic_block *entry_bb,
				 struct bitset *branch_targets)
{
	struct basic_block *bb;
	struct stream stream;

	stream_init(&stream, cu->method->jit_code, cu->method->code_size, &bytecode_stream_ops);

	bb = entry_bb;

	while (stream_has_more(&stream)) {
		if (bc_is_branch(*stream.current))
			bb = do_split(cu, bb, &stream, branch_targets);

		stream_advance(&stream);
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
			bb->successors[0] = new_bb;
			bb = new_bb;
		}
	}
}

int analyze_control_flow(struct compilation_unit *cu)
{
	struct bitset *branch_targets;
	struct basic_block *entry_bb;

	branch_targets = alloc_bitset(cu->method->code_size);
	if (!branch_targets)
		return -ENOMEM;

	entry_bb = get_basic_block(cu, 0, cu->method->code_size);

	split_after_branches(cu, entry_bb, branch_targets);
	split_at_branch_targets(cu, branch_targets);
	update_branch_successors(cu);

	free(branch_targets);

	return 0;
}
