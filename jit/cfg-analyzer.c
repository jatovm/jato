/*
 * Bytecode control-flow analysis.
 *
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <jit/jit-compiler.h>
#include <bytecodes.h>

#include <vm/bitset.h>
#include <vm/stream.h>
#include <vm/vm.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

static unsigned char *bytecode_next_insn(struct stream *stream)
{
	unsigned long opc_size;
		
	opc_size = bytecode_size(stream->current);
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
	unsigned long br_target;
	unsigned long offset;

	offset    = stream_offset(stream);
	br_target = bytecode_br_target(stream->current) + offset;

	set_bit(branch_targets->bits, br_target);
	bb = bb_split(bb, offset + bytecode_size(stream->current));
	list_add_tail(&bb->bb_list_node, &cu->bb_list);

	return bb;
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
		if (bytecode_is_branch(*stream.current))
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
			bb = bb_split(bb, offset);
			list_add_tail(&bb->bb_list_node, &cu->bb_list);
		}
	}
}

int build_cfg(struct compilation_unit *cu)
{
	struct bitset *branch_targets;
	struct basic_block *entry_bb;

	branch_targets = alloc_bitset(cu->method->code_size);
	if (!branch_targets)
		return -ENOMEM;

	entry_bb = alloc_basic_block(cu, 0, cu->method->code_size);
	list_add_tail(&entry_bb->bb_list_node, &cu->bb_list);

	split_after_branches(cu, entry_bb, branch_targets);
	split_at_branch_targets(cu, branch_targets);

	free(branch_targets);

	return 0;
}
