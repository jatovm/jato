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
#include "arch/instruction.h"

#include "vm/stdlib.h"
#include "vm/die.h"

#include "jit/vars.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

static struct live_range *alloc_live_range(unsigned long start, unsigned long end)
{
	struct live_range *range;

	range = malloc(sizeof *range);
	if (!range)
		return NULL;

	range->start = start;
	range->end = end;
	INIT_LIST_HEAD(&range->range_list_node);
	return range;
}

static int split_ranges(struct live_interval *new, struct live_interval *it,
			unsigned long pos)
{
	struct live_range *this;

	list_for_each_entry(this, &it->range_list, range_list_node) {
		struct live_range *next;

		if (!in_range(this, pos))
			continue;

		next = next_range(&it->range_list, this);

		if (this->start == pos) {
			list_move(&this->range_list_node, &new->range_list);
		} else {
			struct live_range *range;

			range = alloc_live_range(pos, this->end);
			if (!range)
				return -ENOMEM;

			this->end = pos;
			list_add(&range->range_list_node, &new->range_list);
		}

		this = next;
		if (!this)
			return 0;

		while (&this->range_list_node != &it->range_list) {
			struct list_head *next;

			next = this->range_list_node.next;

			list_move(&this->range_list_node, new->range_list.prev);
			this = node_to_range(next);
		}

		return 0;
	}

	error("pos is not within an interval live ranges");
}

struct live_interval *alloc_interval(struct var_info *var)
{
	struct live_interval *interval = malloc(sizeof *interval);

	if (interval) {
		interval->var_info = var;
		interval->reg = MACH_REG_UNASSIGNED;
		interval->next_child = NULL;
		interval->prev_child = NULL;
		interval->spill_slot = NULL;
		interval->spill_parent = NULL;
		interval->spill_reload_reg.interval = interval;
		interval->spill_reload_reg.vm_type = var->vm_type;
		INIT_LIST_HEAD(&interval->interval_node);
		INIT_LIST_HEAD(&interval->use_positions);
		INIT_LIST_HEAD(&interval->range_list);
		INIT_LIST_HEAD(&interval->expired_range_list);
		interval->flags = 0;
	}
	return interval;
}

void free_interval(struct live_interval *interval)
{
	if (interval->next_child)
		free_interval(interval->next_child);

	struct live_range *this, *next;
	list_for_each_entry_safe(this, next, &interval->range_list, range_list_node) {
		free(this);
	}

	free(interval);
}

struct live_interval *split_interval_at(struct live_interval *interval,
					unsigned long pos)
{
	struct use_position *this, *next;
	struct live_interval *new;

	assert(pos > interval_start(interval));
	assert(pos < interval_end(interval));

	new = alloc_interval(interval->var_info);
	if (!new)
		return NULL;

	new->reg = MACH_REG_UNASSIGNED;

	if (split_ranges(new, interval, pos)) {
		free(new);
		return NULL;
	}

	if (interval_has_fixed_reg(interval)) {
		new->flags	|= INTERVAL_FLAG_FIXED_REG;
		new->reg = interval->reg;
	}

	list_for_each_entry_safe(this, next, &interval->use_positions, use_pos_list) {
		unsigned long use_pos[2];

		get_lir_positions(this, use_pos);

		if (use_pos[0] < pos)
			continue;

		list_move(&this->use_pos_list, &new->use_positions);
		this->interval = new;
	}

	new->next_child = interval->next_child;
	new->prev_child = interval;
	interval->next_child = new;

	return new;
}

unsigned long next_use_pos(struct live_interval *it, unsigned long pos)
{
	struct use_position *this;
	unsigned long min = LONG_MAX;

	list_for_each_entry(this, &it->use_positions, use_pos_list) {
		unsigned long use_pos[2];
		int nr_use_pos;
		int i;

		nr_use_pos = get_lir_positions(this, use_pos);
		for (i = 0; i < nr_use_pos; i++) {
			if (use_pos[i] < pos)
				continue;

			if (use_pos[i] < min)
				min = use_pos[i];
		}
	}

	return min;
}

struct live_interval *vreg_start_interval(struct compilation_unit *cu, unsigned long vreg)
{
	struct var_info *var;

	var = cu->var_infos;

	while (var) {
		if (var->vreg == vreg)
			break;

		var = var->next;
	}

	if (var == NULL)
		return NULL;

	return var->interval;
}

void interval_expire_ranges_before(struct live_interval *it, unsigned long pos)
{
	struct live_range *range;

	if (pos < interval_start(it) || pos >= interval_end(it))
		return;

	range = interval_first_range(it);

	while (!in_range(range, pos)) {
		struct live_range *next;

		next = next_range(&it->range_list, range);
		if (pos < next->start)
			break;

		list_move(&range->range_list_node, &it->expired_range_list);
		range = next;
	}
}

void interval_restore_expired_ranges(struct live_interval *it)
{
	struct live_range *this, *next;

	list_for_each_entry_safe(this, next, &it->expired_range_list, range_list_node) {
		list_move(&this->range_list_node, &it->range_list);
	}
}

struct live_range *interval_range_at(struct live_interval *it, unsigned long pos)
{
	struct live_range *range;

	if (pos < interval_start(it) || pos >= interval_end(it))
		return NULL;

	range = interval_first_range(it);

	while (range && pos >= range->start) {
		if (in_range(range, pos))
			return range;

		range = next_range(&it->range_list, range);
	}

	return NULL;
}

struct live_interval *interval_child_at(struct live_interval *parent, unsigned long pos)
{
	struct live_interval *it = parent;

	while (it) {
		struct live_range *range;

		range = interval_range_at(it, pos);
		if (range)
			return it;

		it = it->next_child;
	}

	return NULL;
}

bool intervals_intersect(struct live_interval *it1, struct live_interval *it2)
{
	struct live_range *r1, *r2;

	if (interval_start(it1) >= interval_end(it2) ||
	    interval_start(it2) >= interval_end(it1))
		return false;

	if (interval_is_empty(it1) || interval_is_empty(it2))
		return false;

	r1 = interval_first_range(it1);
	r2 = interval_first_range(it2);

	while (r1 && r1->end <= r2->start)
		r1 = next_range(&it1->range_list, r1);

	while (r2 && r2->end <= r1->start)
		r2 = next_range(&it2->range_list, r2);

	while (r1 && r2) {
		if (ranges_intersect(r1, r2))
			return true;

		if (r1->start < r2->start)
			r1 = next_range(&it1->range_list, r1);
		else
			r2 = next_range(&it2->range_list, r2);
	}

	return false;
}

unsigned long
interval_intersection_start(struct live_interval *it1, struct live_interval *it2)
{
	struct live_range *r1, *r2;

	assert(!interval_is_empty(it1) && !interval_is_empty(it2));

	r1 = interval_first_range(it1);
	r2 = interval_first_range(it2);

	while (r1 && r1->end <= r2->start)
		r1 = next_range(&it1->range_list, r1);

	while (r2 && r2->end <= r1->start)
		r2 = next_range(&it2->range_list, r2);

	while (r1 && r2) {
		if (ranges_intersect(r1, r2))
			return range_intersection_start(r1, r2);

		if (r1->start < r2->start)
			r1 = next_range(&it1->range_list, r1);
		else
			r2 = next_range(&it2->range_list, r2);
	}

	error("intervals do not overlap");
}

bool interval_covers(struct live_interval *it, unsigned long pos)
{
	return interval_range_at(it, pos) != NULL;
}

int interval_add_range(struct live_interval *it, unsigned long start,
		       unsigned long end)
{
	struct live_range *range, *next, *new;

	new = alloc_live_range(start, end);
	if (!new)
		return -ENOMEM;

	list_for_each_entry_safe(range, next, &it->range_list, range_list_node) {
		if (range->start > end)
			break;

		if (range->start <= new->end && new->start <= range->end) {
			new->start = min(new->start, range->start);
			new->end = max(new->end, range->end);
			list_del(&range->range_list_node);
			free(range);
		}
	}

	list_for_each_entry_safe(range, next, &it->range_list, range_list_node) {
		if (new->start > range->start)
			continue;

		list_add_tail(&new->range_list_node, &range->range_list_node);
		return 0;
	}

	list_add_tail(&new->range_list_node, &it->range_list);
	return 0;
}

int get_lir_positions(struct use_position *reg, unsigned long *pos)
{
	int nr_pos;

	nr_pos = 0;

	assert(reg->kind);

	if (reg->kind & USE_KIND_INPUT)
		pos[nr_pos++] = reg->insn->lir_pos;

	if (reg->kind & USE_KIND_OUTPUT)
		pos[nr_pos++] = reg->insn->lir_pos + 1;

	return nr_pos;
}
