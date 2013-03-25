/*
 * Copyright (c) 2012  Pekka Enberg
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
 */

#include "jit/compiler.h"

#include "jit/bc-offset-mapping.h"
#include "jit/compilation-unit.h"
#include "jit/basic-block.h"
#include "jit/instruction.h"

#include <assert.h>

static void
insert_reload_after_insn(struct compilation_unit *cu, struct insn *insn, enum vm_type return_type)
{
	int i;

	for (i = 0; i < NR_CALLER_SAVE_REGS; i++) {
		enum machine_reg reg = caller_save_regs[i];
		unsigned long bc_offset;
		struct insn *insn_reload;
		struct stack_slot *slot;
		struct var_info *var;

		if (is_return_reg(reg, return_type))
			continue;

		/* TODO: only spill/reload live registers */

		slot = get_clobber_slot(cu, reg);

		var = get_fixed_var(cu, reg);

		insn_reload = reload_insn(slot, var);

		bc_offset = insn_get_bc_offset(insn);

		insn_set_bc_offset(insn_reload, bc_offset);

		list_add(&insn_reload->insn_list_node, &insn->insn_list_node);
	}
}

static void insert_spill_before_insn(struct compilation_unit *cu, struct insn *insn)
{
	int i;

	for (i = 0; i < NR_CALLER_SAVE_REGS; i++) {
		enum machine_reg reg = caller_save_regs[i];
		unsigned long bc_offset;
		struct insn *insn_spill;
		struct stack_slot *slot;
		struct var_info *var;

		/* TODO: only spill/reload live registers */

		slot = get_clobber_slot(cu, reg);

		var = get_fixed_var(cu, reg);

		insn_spill = spill_insn(var, slot);

		bc_offset = insn_get_bc_offset(insn);

		insn_set_bc_offset(insn_spill, bc_offset);

		list_add_tail(&insn_spill->insn_list_node, &insn->insn_list_node);
	}
}

int mark_clobbers(struct compilation_unit *cu)
{
	struct basic_block *bb;
	struct insn *insn;

	for_each_basic_block(bb, &cu->bb_list) {
		list_for_each_entry(insn, &bb->insn_list, insn_list_node) {
			switch (insn->type) {
			case INSN_SAVE_CALLER_REGS:
				insert_spill_before_insn(cu, insn);
				break;
			case INSN_RESTORE_CALLER_REGS:
				insert_reload_after_insn(cu, insn, J_VOID);
				break;
			case INSN_RESTORE_CALLER_REGS_I32:
				insert_reload_after_insn(cu, insn, J_INT);
				break;
			case INSN_RESTORE_CALLER_REGS_I64:
				insert_reload_after_insn(cu, insn, J_LONG);
				break;
			case INSN_RESTORE_CALLER_REGS_F32:
				insert_reload_after_insn(cu, insn, J_FLOAT);
				break;
			case INSN_RESTORE_CALLER_REGS_F64:
				insert_reload_after_insn(cu, insn, J_DOUBLE);
				break;
			}
		}
	}

	return 0;
}
