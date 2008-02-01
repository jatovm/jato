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
#include <stdlib.h>

static void __init_interval(struct live_interval *interval)
{
	INIT_LIST_HEAD(&interval->interval);
	INIT_LIST_HEAD(&interval->active);
	INIT_LIST_HEAD(&interval->registers);
}

void init_interval(struct live_interval *interval, struct var_info *var)
{
	interval->var_info = var;
	interval->reg = REG_UNASSIGNED;
	interval->range.start = ~0UL;
	interval->range.end = 0UL;
	__init_interval(interval);
}

struct live_interval *split_interval_at(struct live_interval *interval,
					unsigned long pos)
{
	struct live_interval *new;

	new = malloc(sizeof *new);
	if (new) {
		struct register_info *this, *next;

		__init_interval(new);
		new->var_info = interval->var_info;
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
	}
	return new;
}
