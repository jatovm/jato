/*
 * Emits machine code.
 *
 * Copyright (C) 2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include "lib/buffer.h"
#include "vm/class.h"
#include "vm/method.h"
#include "vm/object.h"
#include "vm/die.h"
#include "vm/vm.h"

#include "jit/compilation-unit.h"
#include "jit/basic-block.h"
#include "jit/emit-code.h"
#include "jit/compiler.h"
#include "jit/statement.h"
#include "jit/text.h"

#include "arch/instruction.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

static void emit_monitorenter(struct compilation_unit *cu)
{
	if (vm_method_is_static(cu->method))
		emit_lock(cu->objcode, cu->method->class->object);
	else
		emit_lock_this(cu->objcode);
}

static void emit_monitorexit(struct compilation_unit *cu)
{
	if (vm_method_is_static(cu->method))
		emit_unlock(cu->objcode, cu->method->class->object);
	else
		emit_unlock_this(cu->objcode);
}
static void backpatch_branches(struct buffer *buf,
			       struct list_head *to_backpatch,
			       unsigned long target_offset)
{
	struct insn *this, *next;

	list_for_each_entry_safe(this, next, to_backpatch, branch_list_node) {
		backpatch_branch_target(buf, this, target_offset);
		list_del(&this->branch_list_node);
	}
}

static void backpatch_tableswitch(struct tableswitch *table)
{
	int count;

	count = table->high - table->low + 1;

	for (int i = 0; i < count; i++) {
		int idx = bb_lookup_successor_index(table->src,
						    table->bb_lookup_table[i]);

		if (branch_needs_resolution_block(table->src, idx)) {
			table->lookup_table[i] =
				(void *)table->src->resolution_blocks[idx].addr;
		} else {
			table->lookup_table[i] =
				bb_native_ptr(table->bb_lookup_table[i]);
		}
	}
}

static void backpatch_lookupswitch(struct lookupswitch *table)
{
	for (unsigned int i = 0; i < table->count; i++) {
		int idx = bb_lookup_successor_index(table->src,
						    table->pairs[i].bb_target);

		if (branch_needs_resolution_block(table->src, idx)) {
			table->pairs[i].target =
				(void *)table->src->resolution_blocks[idx].addr;
		} else {
			table->pairs[i].target =
				bb_native_ptr(table->pairs[i].bb_target);
		}
	}
}

static void backpatch_tableswitch_targets(struct compilation_unit *cu)
{
	struct tableswitch *this;

	list_for_each_entry(this, &cu->tableswitch_list, list_node)
	{
		backpatch_tableswitch(this);
	}
}

static void backpatch_lookupswitch_targets(struct compilation_unit *cu)
{
	struct lookupswitch *this;

	list_for_each_entry(this, &cu->lookupswitch_list, list_node)
	{
		backpatch_lookupswitch(this);
	}
}

void emit_body(struct basic_block *bb, struct buffer *buf)
{
	struct insn *insn;

	bb->mach_offset = buffer_offset(buf);
	bb->is_emitted = true;

	backpatch_branches(buf, &bb->backpatch_insns, bb->mach_offset);

	for_each_insn(insn, &bb->insn_list) {
		emit_insn(buf, bb, insn);
	}

	if (opt_trace_machine_code)
		emit_nop(buf);
}

static void emit_resolution_blocks(struct basic_block *bb, struct buffer *buf)
{
	for (unsigned int i = 0; i < bb->nr_successors; i++) {
		struct resolution_block *block;
		unsigned long mach_offset;
		struct insn *insn;

		mach_offset = buffer_offset(buf);
		block = &bb->resolution_blocks[i];
		block->addr = (unsigned long) buffer_ptr(buf) + mach_offset;

		if (list_is_empty(&block->insns))
			continue;

		backpatch_branches(buf, &block->backpatch_insns, mach_offset);

		for_each_insn(insn, &block->insns) {
			emit_insn(buf, NULL, insn);
		}

		emit_insn(buf, NULL, jump_insn(bb->successors[i]));
	}
}

static struct buffer_operations exec_buf_ops = {
	.expand = NULL,
	.free   = NULL,
};

int emit_machine_code(struct compilation_unit *cu)
{
	unsigned long frame_size;
	struct basic_block *bb;
	struct buffer *buf;
	int err = 0;

	buf = __alloc_buffer(&exec_buf_ops);
	if (!buf)
		return warn("out of memory"), -ENOMEM;

	jit_text_lock();

	buf->buf = jit_text_ptr();
	cu->objcode = buf;

	frame_size = frame_locals_size(cu->stack_frame);

	emit_prolog(cu->objcode, frame_size);
	if (method_is_synchronized(cu->method))
		emit_monitorenter(cu);

	if (opt_trace_invoke)
		emit_trace_invoke(cu->objcode, cu);

	for_each_basic_block(bb, &cu->bb_list)
		emit_body(bb, cu->objcode);

	emit_body(cu->exit_bb, cu->objcode);
	if (method_is_synchronized(cu->method))
		emit_monitorexit(cu);
	cu->exit_past_unlock_ptr = buffer_current(cu->objcode);
	emit_epilog(cu->objcode);

	emit_body(cu->unwind_bb, cu->objcode);
	if (method_is_synchronized(cu->method))
		emit_monitorexit(cu);
	cu->unwind_past_unlock_ptr = buffer_current(cu->objcode);
	emit_unwind(cu->objcode);

	for_each_basic_block(bb, &cu->bb_list) {
		emit_resolution_blocks(bb, cu->objcode);
	}

	backpatch_tableswitch_targets(cu);
	backpatch_lookupswitch_targets(cu);

	jit_text_reserve(buffer_offset(cu->objcode));
	jit_text_unlock();

	return err;
}

struct jit_trampoline *alloc_jit_trampoline(void)
{
	struct jit_trampoline *trampoline;

	trampoline = malloc(sizeof(*trampoline));
	if (!trampoline)
		return NULL;

	memset(trampoline, 0, sizeof(*trampoline));

	trampoline->objcode = __alloc_buffer(&exec_buf_ops);
	if (!trampoline->objcode)
		goto failed;

	INIT_LIST_HEAD(&trampoline->fixup_site_list);
	pthread_mutex_init(&trampoline->mutex, NULL);

	return trampoline;

  failed:
	free_jit_trampoline(trampoline);
	return NULL;
}

void free_jit_trampoline(struct jit_trampoline *trampoline)
{
	free_buffer(trampoline->objcode);
	free(trampoline);
}
