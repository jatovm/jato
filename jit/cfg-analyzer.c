/*
 * Bytecode control-flow analysis.
 *
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <jit/compiler.h>

#include <vm/bitset.h>
#include <vm/bytecodes.h>
#include <vm/method.h>
#include <vm/stream.h>
#include <vm/vm.h>

#include <jit/exception.h>

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

static bool is_exception_handler(struct basic_block *bb)
{
	return exception_find_entry(bb->b_parent->method, bb->start) != NULL;
}

static void detect_exception_handlers(struct compilation_unit *cu)
{
	struct basic_block *bb;

	for_each_basic_block(bb, &cu->bb_list) {
		if (is_exception_handler(bb))
			bb->is_eh = true;
	}
}

static int update_branch_successors(struct compilation_unit *cu)
{
	struct basic_block *bb;
	int err = 0;

	for_each_basic_block(bb, &cu->bb_list) {
		struct basic_block *target_bb;

		if (!bb->has_branch)
			continue;

		target_bb = find_bb(cu, bb->br_target_off);
		assert(target_bb != NULL);

		err = bb_add_successor(bb, target_bb);
		if (err)
			break;
	}

	return err;
}

static int split_at_branch_targets(struct compilation_unit *cu,
				    struct bitset *branch_targets)
{
	unsigned long offset;
	int err = 0;

	for (offset = 0; offset < cu->method->code_attribute.code_length;
		offset++)
	{
		struct basic_block *bb;

		if (!test_bit(branch_targets->bits, offset))
			continue;

		bb = find_bb(cu, offset);
		if (bb->start != offset) {
			struct basic_block *new_bb;

			new_bb = bb_split(bb, offset);

			err = bb_add_successor(bb, new_bb);
			if (err)
				break;

			bb = new_bb;
		}
	}

	return err;
}

static inline bool bc_ends_basic_block(unsigned char code)
{
	return bc_is_branch(code) || bc_is_athrow(code) || bc_is_return(code);
}

static int split_after_branches(struct stream *stream,
				 struct basic_block *entry_bb,
				 struct bitset *branch_targets)
{
	struct basic_block *bb;
	int err = 0;

	bb = entry_bb;

	for (; stream_has_more(stream); stream_advance(stream)) {
		unsigned long offset, next_insn_off;
		long br_target_off;
		struct basic_block *new_bb;
		unsigned char *code;

		code = stream->current;

		if (!bc_ends_basic_block(*code))
			continue;

		offset = stream_offset(stream);
		next_insn_off = offset + bc_insn_size(code);

		new_bb = bb_split(bb, next_insn_off);
		if (bc_is_branch(*code) && !bc_is_goto(*code)) {
			err = bb_add_successor(bb, new_bb);
			if (err)
				break;
		}

		if (bc_is_branch(*code)) {
			br_target_off = bc_target_off(code) + offset;

			bb->br_target_off = br_target_off;
			bb->has_branch = true;

			set_bit(branch_targets->bits, br_target_off);
		}

		bb = new_bb;
	}

	return err;
}

static int update_athrow_successors(struct stream *stream,
				     struct compilation_unit *cu)
{
	struct vm_method *method;
	int err = 0;

	method = cu->method;

	for (; stream_has_more(stream); stream_advance(stream)) {
		struct basic_block *bb;
		unsigned long offset;
		int i;

		offset = stream_offset(stream);

		if (!bc_is_athrow(*stream->current))
			continue;

		bb = find_bb(cu, offset);

		for (i = 0; i < method->code_attribute.exception_table_length;
			i++)
		{
			struct cafebabe_code_attribute_exception *eh;

			eh = &method->code_attribute.exception_table[i];

			if (exception_covers(eh, offset)) {
				struct basic_block *eh_bb;

				eh_bb = find_bb(cu, eh->handler_pc);
				assert(eh_bb != NULL);

				err = bb_add_successor(bb, eh_bb);
				if (err)
					goto out;
			}
		}
	}

 out:
	return err;
}

static bool all_exception_handlers_have_bb(struct compilation_unit *cu)
{
	struct vm_method *method = cu->method;
	int i;

	for (i = 0; i < method->code_attribute.exception_table_length; i++) {
		struct cafebabe_code_attribute_exception *eh;
		struct basic_block *bb;

		eh = &method->code_attribute.exception_table[i];
		bb = find_bb(cu, eh->handler_pc);

		if (bb == 0 || bb->start != eh->handler_pc)
			return false;

	}

	return true;
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

static void bytecode_stream_init(struct stream *stream, struct vm_method *method)
{
	stream_init(stream,
		    method->code_attribute.code,
		    method->code_attribute.code_length,
		    &bytecode_stream_ops);
}

int analyze_control_flow(struct compilation_unit *cu)
{
	struct bitset *branch_targets;
	struct stream stream;
	int err = 0;

	branch_targets = alloc_bitset(cu->method->code_attribute.code_length);
	if (!branch_targets)
		return -ENOMEM;

	bytecode_stream_init(&stream, cu->method);

	cu->entry_bb = get_basic_block(cu, 0, cu->method->code_attribute.code_length);

	err = split_after_branches(&stream, cu->entry_bb, branch_targets);
	if (err)
		goto out;

	err = split_at_branch_targets(cu, branch_targets);
	if (err)
		goto out;

	err = update_branch_successors(cu);
	if (err)
		goto out;

	detect_exception_handlers(cu);

	bytecode_stream_init(&stream, cu->method);
	err = update_athrow_successors(&stream, cu);
	if (err)
		goto out;

	/*
	 * This checks whether every exception handler has its own
	 * basic block which starts at handler_pc. There always should
	 * be a branch, athrow or return before each exception handler
	 * which will guarantee a basic block starts at exception handler.
	 */
	assert(all_exception_handlers_have_bb(cu));

 out:
	free(branch_targets);

	return err;
}
