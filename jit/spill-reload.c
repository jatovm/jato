/*
 * Copyright (c) 2008  Pekka Enberg
 * 
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */

#include "jit/compilation-unit.h"
#include "jit/instruction.h"
#include "jit/stack-slot.h"
#include "jit/compiler.h"

#include "arch/instruction.h"

#include "lib/bitset.h"
#include "lib/buffer.h"

#include "vm/die.h"
#include "vm/trace.h"

#include <malloc.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

struct live_interval_mapping {
	struct live_interval *from, *to;
};

static struct insn *first_insn(struct compilation_unit *cu, struct live_interval *interval)
{
	struct insn *ret;

	ret = radix_tree_lookup(cu->lir_insn_map, range_first_insn_pos(&interval->range));
	assert(ret != NULL);

	return ret;
}

static struct insn *last_insn(struct compilation_unit *cu, struct live_interval *interval)
{
	struct insn *ret;

	ret = radix_tree_lookup(cu->lir_insn_map, range_last_insn_pos(&interval->range));
	assert(ret != NULL);

	return ret;
}

/**
 * Returns the node before which spill instructions should be inserted
 * when they are supposed to be executed just before control leaves
 * given basic block. When basic block is ended with a branch
 * instruction it returns node of that branch; otherwise it returns
 * the next node.
 */
static struct list_head *bb_last_spill_node(struct basic_block *bb)
{
	struct insn *last;

	/*
	 * basic block's instruction list might not be empty and
	 * contain only spill/reload instructions. In this situation
	 * we also consider basic block as empty (no lir positions) so
	 * we don't rely on ->insn_list here.
	 */
	if (bb->end_insn == bb->start_insn)
		return &bb->insn_list;

	last = radix_tree_lookup(bb->b_parent->lir_insn_map, bb->end_insn - 2);
	assert(last);

	if (insn_is_branch(last))
		return &last->insn_list_node;

	return last->insn_list_node.next;
}

static struct stack_slot *
spill_interval(struct live_interval *interval,
	       struct compilation_unit *cu,
	       struct list_head *spill_before,
	       unsigned long bc_offset)
{
	struct stack_slot *slot;
	struct insn *spill;

	slot = get_spill_slot_32(cu->stack_frame);
	if (!slot)
		return NULL;

	spill = spill_insn(&interval->spill_reload_reg, slot);
	if (!spill)
		return NULL;

	spill->bytecode_offset = bc_offset;

	list_add_tail(&spill->insn_list_node, spill_before);
	return slot;
}

static int
insert_spill_insn(struct live_interval *interval, struct compilation_unit *cu)
{
	struct insn *last;

	last = last_insn(cu, interval);
	if (!insn_is_branch(last))
		last = next_insn(last);

	interval->spill_slot = spill_interval(interval, cu, &last->insn_list_node,
					      last->bytecode_offset);
	if (!interval->spill_slot)
		return warn("out of memory"), -ENOMEM;

	return 0;
}

static int insert_reload_insn(struct live_interval *interval,
			      struct compilation_unit *cu,
			      struct stack_slot *from,
			      struct insn *first)
{
	struct insn *reload;

	reload = reload_insn(from, &interval->spill_reload_reg);
	if (!reload)
		return warn("out of memory"), -ENOMEM;

	reload->bytecode_offset = first->bytecode_offset;

	list_add_tail(&reload->insn_list_node, &first->insn_list_node);

	return 0;
}

static int
insert_copy_slot_insn(struct live_interval *interval,
		      struct compilation_unit *cu,
		      struct stack_slot *from,
		      struct stack_slot *to,
		      struct list_head *push_before,
		      unsigned long bc_offset)
{
	struct insn *push, *pop;

	push = push_slot_insn(from);
	if (!push)
		return warn("out of memory"), -ENOMEM;
	push->bytecode_offset = bc_offset;

	pop = pop_slot_insn(to);
	if (!pop) {
		free_insn(push);
		return warn("out of memory"), -ENOMEM;
	}
	pop->bytecode_offset = bc_offset;

	list_add_tail(&push->insn_list_node, push_before);
	list_add(&pop->insn_list_node, &push->insn_list_node);

	return 0;

}

static int __insert_spill_reload_insn(struct live_interval *interval, struct compilation_unit *cu)
{
	int err = 0;

	if (range_is_empty(&interval->range))
		goto out;

	if (interval->need_reload) {
		/*
		 * Intervals which start with a DEF position (odd
		 * numbers) should not be reloaded. One reason for
		 * this is that they do not have to because register
		 * content is overriden. Another reason is that we
		 * can't insert a reload instruction in the middle of
		 * instruction.
		 */
		assert((interval->range.start & 1) == 0);

		err = insert_reload_insn(interval, cu,
				interval->spill_parent->spill_slot,
				first_insn(cu, interval));
		if (err)
			goto out;
	}

	if (interval->need_spill) {
		err = insert_spill_insn(interval, cu);
		if (err)
			goto out;
	}
out:
	return err;
}

static void insert_mov_insns(struct compilation_unit *cu,
			     struct live_interval_mapping *mappings,
			     int nr_mapped,
			     struct basic_block *from_bb,
			     struct basic_block *to_bb)
{
	struct live_interval *from_it, *to_it;
	struct stack_slot *slots[nr_mapped];
	struct list_head *spill_before;
	unsigned long bc_offset;
	int i;

	spill_before = bb_last_spill_node(from_bb);
	bc_offset = from_bb->end - 1;

	/* Spill all intervals that have to be resolved */
	for (i = 0; i < nr_mapped; i++) {
		from_it		= mappings[i].from;

		if (from_it->need_spill && from_it->range.end < from_bb->end_insn) {
			slots[i] = from_it->spill_slot;
		} else {
			slots[i] = spill_interval(from_it, cu, spill_before, bc_offset);
		}
	}

	/* Reload those intervals into their new location */
	for (i = 0; i < nr_mapped; i++) {
		to_it		= mappings[i].to;

		if (to_it->need_reload && to_it->range.start >= to_bb->start_insn) {
			insert_copy_slot_insn(mappings[i].to, cu, slots[i],
					      to_it->spill_parent->spill_slot,
					      spill_before, bc_offset);
			continue;
		}

		/*
		 * Reload instructions should be inserted to per-edge
		 * resolution blocks because register we are reloading
		 * to might be already allocated to another interval
		 * at this position. Especially it can be used in a
		 * branch instruction which ends @from_bb basic block
		 * (see tableswitch).
		 */
		struct insn *reload;
		int idx;

		idx = bb_lookup_successor_index(from_bb, to_bb);
		reload = reload_insn(slots[i], &to_it->spill_reload_reg);
		list_add_tail(&reload->insn_list_node,
			      &from_bb->resolution_blocks[idx].insns);
	}
}

static void maybe_add_mapping(struct live_interval_mapping *mappings,
			      struct compilation_unit *cu,
			      struct basic_block *from,
			      struct basic_block *to,
			      unsigned long vreg,
			      int *nr_mapped)
{
	struct live_interval *parent_it;
	struct live_interval *from_it, *to_it;

	parent_it	= vreg_start_interval(cu, vreg);
	from_it		= interval_child_at(parent_it, from->end_insn - 1);
	to_it		= interval_child_at(parent_it, to->start_insn);

	/*
	 * The intervals are the same on both sides of the basic block edge.
	 */
	if (from_it == to_it)
		return;

	/*
	 * We seem to have some vregs that are alive at the beginning of a
	 * basic block but have no interval covering them. In that case, no
	 * mov instruction is to be inserted.
	 */
	if (!from_it || !to_it)
		return;

	/*
	 * If any of the intervals have no register assigned at this point, it
	 * is because the register allocator found out the interval is useless.
	 * In that case, we need to find what the *real* destination interval
	 * is.
	 */
	while (to_it->reg == MACH_REG_UNASSIGNED) {
		to_it = to_it->next_child;
		assert(to_it);
	}

	/*
	 * Same goes for the source interval, but we do not have a prev_child
	 * field, so we need to cheat a bit.
	 */
	while (from_it->reg == MACH_REG_UNASSIGNED) {
		from_it = from_it->prev_child;
		assert(from_it);
	}

	assert(to_it);
	assert(from_it);

	mappings[*nr_mapped].from	= from_it;
	mappings[*nr_mapped].to		= to_it;
	(*nr_mapped)++;
}

static int resolve_data_flow(struct compilation_unit *cu)
{
	struct basic_block *from;
	unsigned long vreg;

	/*
	 * This implements the data flow resolution algorithm described in
	 * Section 5.8 ("Resolving the Data Flow") of Wimmer 2004.
	 */
	for_each_basic_block(from, &cu->bb_list) {
		unsigned long rb_size;
		unsigned int i;

		rb_size = sizeof(struct resolution_block) * from->nr_successors;
		from->resolution_blocks = malloc(rb_size);
		if (!from->resolution_blocks)
			return -ENOMEM;

		for (i = 0; i < from->nr_successors; i++) {
			struct live_interval_mapping mappings[cu->nr_vregs];
			struct basic_block *to;
			int nr_mapped = 0;

			resolution_block_init(&from->resolution_blocks[i]);

			if (cu->nr_vregs == 0)
				continue;

			memset(mappings, 0, sizeof(mappings));

			to = from->successors[i];

			for (vreg = 0; vreg < cu->nr_vregs; vreg++) {
				if (test_bit(to->live_in_set->bits, vreg)) {
					maybe_add_mapping(mappings, cu, from, to, vreg, &nr_mapped);
				}
			}

			insert_mov_insns(cu, mappings, nr_mapped, from, to);
		}
	}

	return 0;
}

int insert_spill_reload_insns(struct compilation_unit *cu)
{
	struct var_info *var;
	int err = 0;

	for_each_variable(var, cu->var_infos) {
		struct live_interval *interval;

		for (interval = var->interval; interval != NULL; interval = interval->next_child) {
			err = __insert_spill_reload_insn(interval, cu);
			if (err)
				break;
		}
	}

	/*
	 * Make sure intervals spilled across basic block boundaries will be
	 * reloaded correctly.
	 */
	err = resolve_data_flow(cu);

	return err;
}
