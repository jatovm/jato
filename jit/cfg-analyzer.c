/*
 * Bytecode control-flow analysis.
 *
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include "jit/compiler.h"

#include "lib/bitset.h"
#include "vm/bytecode.h"
#include "vm/method.h"
#include "vm/stream.h"
#include "vm/vm.h"

#include "jit/exception.h"

#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

static bool is_exception_handler(struct basic_block *bb)
{
	return lookup_eh_entry(bb->b_parent->method, bb->start) != NULL;
}

static void detect_exception_handlers(struct compilation_unit *cu)
{
	struct basic_block *bb;

	for_each_basic_block(bb, &cu->bb_list)
		bb->is_eh = is_exception_handler(bb);
}

static int split_at_exception_handlers(struct compilation_unit *cu)
{
	struct vm_method *method = cu->method;
	int i, err = 0;

	for (i = 0; i < method->code_attribute.exception_table_length; i++) {
		struct cafebabe_code_attribute_exception *eh;
		struct basic_block *bb;

		eh = &method->code_attribute.exception_table[i];

		bb = find_bb(cu, eh->handler_pc);
		if (!bb) {
			err = -1;
			break;
		}

		assert(bb->is_eh);

		if (bb->start != eh->handler_pc) {
			struct basic_block *new_bb;

			new_bb = bb_split(bb, eh->handler_pc);
			if (!new_bb) {
				err = -1;
				break;
			}
			new_bb->is_eh = true;

			err = bb_add_successor(bb, new_bb);
			if (err)
				break;
		}
	}

	return err;
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

		if (bb_successors_contains(bb, target_bb))
			continue;

		err = bb_add_successor(bb, target_bb);
		if (err)
			break;
	}

	return err;
}

static int split_at_branch_targets(struct compilation_unit *cu, struct bitset *branch_targets)
{
	unsigned long offset;
	int err = 0;

	for (offset = 0; offset < cu->method->code_attribute.code_length; offset++) {
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

static int split_after_branches(const unsigned char *code,
				unsigned long code_length,
				struct basic_block *entry_bb,
				struct bitset *branch_targets)
{
	struct basic_block *bb;
	unsigned long offset;
	int err = 0;

	bb = entry_bb;

	bytecode_for_each_insn(code, code_length, offset) {
		struct basic_block *new_bb = NULL;
		unsigned long next_insn_off;
		unsigned char opcode;
		long br_target_off;

		opcode = code[offset];

		if (!bc_ends_basic_block(opcode))
			continue;

		next_insn_off = offset + bc_insn_size(code, offset);

		if (next_insn_off != bb->end) {
			new_bb = bb_split(bb, next_insn_off);

			if (bc_is_branch(opcode) && !bc_is_goto(opcode)) {
				err = bb_add_successor(bb, new_bb);
				if (err)
					break;
			}
		}

		if (opcode == OPC_TABLESWITCH) {
			struct tableswitch_info info;

			get_tableswitch_info(code, offset, &info);

			/*
			 * We mark tableswitch (and lookupswitch)
			 * targets to be split but we do not connect
			 * them to this basic block because it will be
			 * later split in convert_tableswitch().
			 */
			set_bit(branch_targets->bits,
				offset + info.default_target);

			for (unsigned int i = 0; i < info.count; i++) {
				int32_t target;

				target = read_s32(info.targets + i * 4);
				set_bit(branch_targets->bits, offset + target);
			}
		} else if (opcode == OPC_LOOKUPSWITCH) {
			struct lookupswitch_info info;

			get_lookupswitch_info(code, offset, &info);

			set_bit(branch_targets->bits,
				offset + info.default_target);

			for (unsigned int i = 0; i < info.count; i++) {
				int32_t target;

				target = read_lookupswitch_target(&info, i);
				set_bit(branch_targets->bits, offset + target);
			}
		} else if (bc_is_branch(opcode)) {
			br_target_off = bc_target_off(&code[offset]) + offset;

			bb->br_target_off = br_target_off;
			bb->has_branch = true;

			set_bit(branch_targets->bits, br_target_off);
		} else if (bc_is_return(opcode)) {
			bb->has_return = true;
		} else if (bc_is_athrow(opcode)) {
			bb->has_athrow = true;
		}

		if (new_bb)
			bb = new_bb;
	}

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

		if (bb == NULL || bb->start != eh->handler_pc || !bb->is_eh)
			return false;
	}

	return true;
}

int analyze_control_flow(struct compilation_unit *cu)
{
	struct bitset *branch_targets;
	const unsigned char *code;
	int code_length;
	int err = 0;

	code = cu->method->code_attribute.code;
	code_length = cu->method->code_attribute.code_length;

	branch_targets = alloc_bitset(code_length);
	if (!branch_targets)
		return warn("out of memory"), -ENOMEM;

	cu->entry_bb = get_basic_block(cu, 0, code_length);

	err = split_after_branches(code, code_length, cu->entry_bb,
				   branch_targets);
	if (err)
		goto out;

	err = split_at_branch_targets(cu, branch_targets);
	if (err)
		goto out;

	err = update_branch_successors(cu);
	if (err)
		goto out;

	detect_exception_handlers(cu);

	err = split_at_exception_handlers(cu);
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
