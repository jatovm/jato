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
#include <arch/instruction.h>
#include <jit/vars.h>
#include <vm/stdlib.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

struct live_interval *alloc_interval(struct var_info *var)
{
	struct live_interval *interval = zalloc(sizeof *interval);
	if (interval) {
		interval->var_info = var;
		interval->reg = REG_UNASSIGNED;
		interval->range.start = ~0UL;
		interval->range.end = 0UL;
		INIT_LIST_HEAD(&interval->interval);
		INIT_LIST_HEAD(&interval->active);
		INIT_LIST_HEAD(&interval->registers);
	}
	return interval;
}

void free_interval(struct live_interval *interval)
{
	free(interval->insn_array);
	free(interval);
}

static int split_insn_array(struct live_interval *src, struct live_interval *dst)
{
	unsigned long src_len, dst_len, size;

	src_len = range_len(&src->range);
	dst_len = range_len(&dst->range);

	size = dst_len * sizeof(struct insn *);
	dst->insn_array = zalloc(size);
	if (!dst->insn_array)
		return -ENOMEM;

	memcpy(dst->insn_array, src->insn_array + src_len, size);

	return 0;
}

struct live_interval *split_interval_at(struct live_interval *interval,
					unsigned long pos)
{
	struct register_info *this, *next;
	struct live_interval *new;
	int err;

	new = alloc_interval(interval->var_info);
	if (!new)
		return NULL;

	new->reg = interval->reg;
	new->range.start = pos;
	new->range.end = interval->range.end;
	interval->range.end = pos;

	list_for_each_entry_safe(this, next, &interval->registers, reg_list) {
		if (this->insn->lir_pos < pos)
			continue;

		list_move(&this->reg_list, &new->registers);
		this->interval = new;
	}
	err = split_insn_array(interval, new);
	if (err) {
		free_interval(new);
		return NULL;
	}
	return new;
}
