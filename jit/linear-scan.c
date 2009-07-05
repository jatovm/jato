/*
 * Linear scan register allocation
 * Copyright © 2007-2008  Pekka Enberg
 * Copyright © 2008	  Arthur Huillet
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
 *
 * For more details on the linear scan register allocation algorithm used here,
 * please refer to the following paper:
 *
 *   Wimmer, C. and Mössenböck, H. 2005. Optimized interval splitting
 *   in a linear scan register allocator. In Proceedings of the 1st
 *   ACM/USENIX international Conference on Virtual Execution
 *   Environments (Chicago, IL, USA, June 11 - 12, 2005). VEE '05. ACM
 *   Press, New York, NY, 132-141.
 */

#include "jit/compiler.h"
#include "jit/vars.h"
#include "vm/bitset.h"
#include <errno.h>
#include <limits.h>
#include <stdlib.h>

static void set_free_pos(unsigned long *free_until_pos, enum machine_reg reg,
			 unsigned long pos)
{
	/*
	 * If reg is greater than or equal to NR_REGISTERS, it's
	 * not a general purpose register and thus not available for
	 * allocation so it's safe to just ignore it.
	 */
	if (reg >= NR_REGISTERS)
		return;

	/*
	 * If the position for one register is set multiple times the minimum
	 * of all positions is used. This can happen when many inactive
	 * intervals have the same register assigned.
	 */
	if (free_until_pos[reg] < pos)
		return;

	free_until_pos[reg] = pos;
}

static void set_use_pos(unsigned long *use_pos, enum machine_reg reg,
			unsigned long pos)
{
	/*
	 * This function does the same as set_free_pos so we call this directly
	 */
	set_free_pos(use_pos, reg, pos);
}

static void set_block_pos(unsigned long *block_pos, unsigned long *use_pos,
			  enum machine_reg reg, unsigned long pos)
{
	/*
	 * This function does the same as set_free_pos so we call this directly
	 */
	set_free_pos(block_pos, reg, pos);
	set_free_pos(use_pos, reg, pos);
}

/* Find the register that is used the latest.  */
static enum machine_reg pick_register(unsigned long *free_until_pos, enum machine_reg_type type)
{
	unsigned long max_pos = 0;
	int i, ret = 0;

	for (i = 0; i < NR_REGISTERS; i++) {
		unsigned long pos = free_until_pos[i];

		if (reg_type(i) != type)
			continue;

		if (pos > max_pos) {
			max_pos = pos;
			ret = i;
		}
	}
	return ret;
}

/* Inserts to a list of intervals sorted by increasing start position.  */
static void
insert_to_list(struct live_interval *interval, struct list_head *interval_list)
{
	struct live_interval *this;

	/*
	 * If we find an existing interval, that starts _after_ the
	 * new interval, add ours before that.
	 */
	list_for_each_entry(this, interval_list, interval_node) {
		if (interval->range.start < this->range.start) {
			list_add_tail(&interval->interval_node, &this->interval_node);
			return;
		}
	}

	/*
	 * Otherwise the new interval goes to the end of the list.
	 */
	list_add_tail(&interval->interval_node, interval_list);
}

static void __spill_interval_intersecting(struct live_interval *current,
					  enum machine_reg reg,
					  struct live_interval *it,
					  struct list_head *unhandled)
{
	struct live_interval *new;
	unsigned long next_pos;

	if (it->reg != reg)
		return;

	if (!ranges_intersect(&it->range, &current->range))
		return;

	new = split_interval_at(it, current->range.start);
	it->need_spill = true;

	next_pos = next_use_pos(new, new->range.start);

	if (next_pos == LONG_MAX)
		return;

	new = split_interval_at(new, next_pos);
	mark_need_reload(new, it);

	insert_to_list(new, unhandled);
}

static void spill_all_intervals_intersecting(struct live_interval *current,
					     enum machine_reg reg,
					     struct list_head *active,
					     struct list_head *inactive,
					     struct list_head *unhandled)
{
	struct live_interval *it;

	list_for_each_entry(it, active, interval_node) {
		__spill_interval_intersecting(current, reg, it, unhandled);
	}

	list_for_each_entry(it, inactive, interval_node) {
		__spill_interval_intersecting(current, reg, it, unhandled);
	}

}

static void allocate_blocked_reg(struct live_interval *current,
				 struct list_head *active,
				 struct list_head *inactive,
				 struct list_head *unhandled)
{
	unsigned long use_pos[NR_REGISTERS], block_pos[NR_REGISTERS];
	struct live_interval *it, *new;
	int i;
	enum machine_reg reg;

	for (i = 0; i < NR_REGISTERS; i++) {
		use_pos[i] = LONG_MAX;
		block_pos[i] = LONG_MAX;
	}

	list_for_each_entry(it, active, interval_node) {
		unsigned long pos;

		if (it->fixed_reg)
			continue;

		pos = next_use_pos(it, current->range.start);
		set_use_pos(use_pos, it->reg, pos);
	}

	list_for_each_entry(it, inactive, interval_node) {
		unsigned long pos;

		if (it->fixed_reg)
			continue;

		if (ranges_intersect(&it->range, &current->range)) {
			pos = next_use_pos(it, current->range.start);
			set_use_pos(use_pos, it->reg, pos);
		}
	}

	list_for_each_entry(it, active, interval_node) {
		if (!it->fixed_reg)
			continue;

		set_block_pos(block_pos, use_pos, it->reg, 0);
	}

	list_for_each_entry(it, inactive, interval_node) {
		if (!it->fixed_reg)
			continue;

		if (ranges_intersect(&it->range, &current->range)) {
			unsigned long pos;

			pos = range_intersection_start(&it->range, &current->range);
			set_block_pos(block_pos, use_pos, it->reg, pos);
		}
	}

	reg = pick_register(use_pos, current->var_info->type);
	if (use_pos[reg] < next_use_pos(current, 0)) {
		unsigned long pos;

		/*
		 * All active and inactive intervals are used before current,
		 * so it is best to spill current itself
		 */
		pos = next_use_pos(current, current->range.start);
		new = split_interval_at(current, pos);
		mark_need_reload(new, current);
		insert_to_list(new, unhandled);
		current->need_spill = 1;
	} else if (block_pos[reg] > current->range.end) {
		/* Spilling made a register free for the whole current */
		current->reg = reg;
		spill_all_intervals_intersecting(current, reg, active,
						 inactive, unhandled);
	} else {
		new = split_interval_at(current, block_pos[reg]);
		insert_to_list(new, unhandled);

		current->reg = reg;
		spill_all_intervals_intersecting(current, reg, active,
						 inactive, unhandled);
	}
}

static void try_to_allocate_free_reg(struct live_interval *current,
				     struct list_head *active,
				     struct list_head *inactive,
				     struct list_head *unhandled)
{
	unsigned long free_until_pos[NR_REGISTERS];
	struct live_interval *it, *new;
	enum machine_reg reg;
	int i;

	for (i = 0; i < NR_REGISTERS; i++)
		free_until_pos[i] = LONG_MAX;

	list_for_each_entry(it, active, interval_node) {
		set_free_pos(free_until_pos, it->reg, 0);
	}

	list_for_each_entry(it, inactive, interval_node) {
		if (ranges_intersect(&it->range, &current->range)) {
			unsigned long pos;

			pos = range_intersection_start(&it->range, &current->range);
			set_free_pos(free_until_pos, it->reg, pos);
		}
	}

	reg = pick_register(free_until_pos, current->var_info->type);
	if (free_until_pos[reg] == 0) {
		/*
		 * No register available without spilling.
		 */
		return;
	}

	if (current->range.end < free_until_pos[reg]) {
		/*
		 * Register available for the full interval.
		 */
		current->reg = reg;
	} else {
		/*
		 * Register available for the first part of the interval.
		 */
		new = split_interval_at(current, free_until_pos[reg]);
		mark_need_reload(new, current);
		insert_to_list(new, unhandled);
		current->reg = reg;
		current->need_spill = 1;
	}
}

int allocate_registers(struct compilation_unit *cu)
{
	struct list_head unhandled = LIST_HEAD_INIT(unhandled);
	struct list_head inactive = LIST_HEAD_INIT(inactive);
	struct list_head active = LIST_HEAD_INIT(active);
	struct live_interval *current;
	struct bitset *registers;
	struct var_info *var;

	registers = alloc_bitset(NR_REGISTERS);
	if (!registers)
		return -ENOMEM;

	bitset_set_all(registers);

	/*
	 * Fixed intervals are placed on the inactive list initially so that
	 * the allocator can avoid conflicts when allocating a register for a
	 * non-fixed interval.
	 */
	for_each_variable(var, cu->var_infos) {
		if (var->interval->fixed_reg)
			insert_to_list(var->interval, &inactive);
	}

	for_each_variable(var, cu->var_infos) {
		if (!var->interval->fixed_reg)
			insert_to_list(var->interval, &unhandled);
	}

	while (!list_is_empty(&unhandled)) {
		struct live_interval *it, *prev;
		unsigned long position;

		current = list_first_entry(&unhandled, struct live_interval, interval_node);
		list_del(&current->interval_node);
		position = current->range.start;

		list_for_each_entry_safe(it, prev, &active, interval_node) {
			if (it->range.end <= position) {
				/*
				 * Remove handled interval from active list.
				 */
				list_del(&it->interval_node);
				continue;
			}
			if (!in_range(&it->range, position))
				list_move(&it->interval_node, &inactive);
		}

		list_for_each_entry_safe(it, prev, &inactive, interval_node) {
			if (it->range.end <= position) {
				/*
				 * Remove handled interval from inactive list.
				 */
				list_del(&it->interval_node);
				continue;
			}
			if (in_range(&it->range, position))
				list_move(&it->interval_node, &active);
		}

		/*
		 * Don't allocate registers for fixed intervals.
		 */
		if (!current->fixed_reg) {
			try_to_allocate_free_reg(current, &active, &inactive, &unhandled);

			if (current->reg == REG_UNASSIGNED)
				allocate_blocked_reg(current, &active, &inactive, &unhandled);
		}
		if (current->reg != REG_UNASSIGNED)
			list_add(&current->interval_node, &active);
	}
	free(registers);

	return 0;
}
