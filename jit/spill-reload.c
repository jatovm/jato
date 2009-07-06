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
#include "jit/stack-slot.h"
#include "jit/compiler.h"

#include "arch/instruction.h"

#include <errno.h>

static struct insn *last_insn(struct live_interval *interval)
{
	unsigned long end;
	struct insn *ret;

	end = range_len(&interval->range) - 1;
	ret = interval->insn_array[end];
	assert(ret != NULL);

	return ret;
}

static int insert_spill_insn(struct live_interval *interval, struct compilation_unit *cu)
{
	struct insn *last = last_insn(interval);
	struct stack_slot *slot;
	struct var_info *reg;
	struct insn *spill;

	/*
	 * We've already done register allocation, so use fixed registers for
	 * spilling and reloading.
	 */
	reg = get_fixed_var(cu, interval->reg);

	slot = get_spill_slot_32(cu->stack_frame);
	if (!slot)
		return -ENOMEM;

	spill = spill_insn(reg, slot);
	if (!spill)
		return -ENOMEM;

	interval->spill_slot = slot;

	spill->bytecode_offset = last->bytecode_offset;

	list_add(&spill->insn_list_node, &last->insn_list_node);

	return 0;
}

static struct insn *first_insn(struct live_interval *interval)
{
	struct insn *ret;

	ret = interval->insn_array[0];
	assert(ret != NULL);

	return ret;
}

static int insert_reload_insn(struct live_interval *interval, struct compilation_unit *cu)
{
	struct insn *first = first_insn(interval);
	struct insn *reload;
	struct var_info *reg;

	reg = get_fixed_var(cu, interval->reg);
	reload = reload_insn(interval->spill_parent->spill_slot, reg);
	if (!reload)
		return -ENOMEM;

	reload->bytecode_offset = first->bytecode_offset;

	list_add_tail(&reload->insn_list_node, &first->insn_list_node);

	return 0;
}

static int __insert_spill_reload_insn(struct live_interval *interval, struct compilation_unit *cu)
{
	int err = 0;

	if (range_is_empty(&interval->range))
		goto out;

	if (interval->need_reload) {
		err = insert_reload_insn(interval, cu);
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
	return err;
}
