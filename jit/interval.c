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

struct live_interval *alloc_interval(struct var_info *var)
{
	struct live_interval *interval = zalloc(sizeof *interval);
	if (interval) {
		interval->var_info = var;
		interval->reg = MACH_REG_UNASSIGNED;
		interval->fixed_reg = false;
		interval->range.start = ~0UL;
		interval->range.end = 0UL;
		interval->spill_reload_reg.interval = interval;
		INIT_LIST_HEAD(&interval->interval_node);
		INIT_LIST_HEAD(&interval->use_positions);
	}
	return interval;
}

void free_interval(struct live_interval *interval)
{
	if (interval->next_child)
		free_interval(interval->next_child);

	free(interval);
}

struct live_interval *split_interval_at(struct live_interval *interval,
					unsigned long pos)
{
	struct use_position *this, *next;
	struct live_interval *new;

	new = alloc_interval(interval->var_info);
	if (!new)
		return NULL;

	new->reg = MACH_REG_UNASSIGNED;
	new->range.start = pos;
	new->range.end = interval->range.end;
	interval->range.end = pos;

	list_for_each_entry_safe(this, next, &interval->use_positions, use_pos_list) {
		if (lir_position(this) < pos)
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
		if (lir_position(this) < pos)
			continue;

		if (lir_position(this) < min)
			min = lir_position(this);
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

struct live_interval *interval_child_at(struct live_interval *parent, unsigned long pos)
{
	struct live_interval *it = parent;

	while (it) {
		if (in_range(&it->range, pos))
			return it;

		it = it->next_child;
	}

	return NULL;
}
