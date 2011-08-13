/*
 * Conversion from LIR to SSA, optimizations and conversion from SSA to LIR
 * Copyright (c) 2011 Ana Farcasi
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

#include "jit/compilation-unit.h"
#include "jit/compiler.h"
#include "jit/instruction.h"
#include "jit/ssa.h"
#include "jit/vars.h"

#include "lib/bitset.h"
#include "lib/radix-tree.h"

#include <stdio.h>

void abc_removal(struct compilation_unit *cu)
{
	struct basic_block *bb;
	struct insn *insn, *tmp;
	struct reg_value_acc regs_value[cu->ssa_nr_vregs];
	struct array_size_acc arrays_value[cu->ssa_nr_vregs];
	int index = 0, nr_array_check = 0;

	for (unsigned i = 0; i < cu->ssa_nr_vregs; i++) {
		regs_value[i].active = false;

		arrays_value[i].active = false;
	}

	for_each_basic_block(bb, &cu->bb_list)
		list_for_each_entry(insn, &bb->insn_list, insn_list_node)
			if (insn_is_call_to(insn, vm_object_alloc_primitive_array))
				nr_array_check++;

	struct insn *delete[nr_array_check];

	for_each_basic_block(bb, &cu->bb_list) {
		list_for_each_entry_safe(insn, tmp, &bb->insn_list, insn_list_node) {
			if (insn_is_mov_imm_reg(insn)) {
				uint32_t reg = insn->dest.reg.interval->var_info->vreg;

				regs_value[reg].active = true;
				regs_value[reg].val = insn->src.imm;
			}

			if (insn_is_call(insn)
					&& insn->operand.rel == (unsigned long)vm_object_alloc_primitive_array) {
				struct insn *insn_push_size, *insn_mov_eax_array;
				uint32_t size_reg, array_reg;

				insn_push_size = list_entry(insn->insn_list_node.prev->prev->prev,
								struct insn, insn_list_node);
				size_reg = insn_push_size->src.reg.interval->var_info->vreg;

				insn_mov_eax_array = list_entry(insn->insn_list_node.next,
								struct insn, insn_list_node);
				array_reg = insn_mov_eax_array->dest.reg.interval->var_info->vreg;

				if (regs_value[size_reg].active) {
					arrays_value[array_reg].active = true;
					arrays_value[array_reg].size = regs_value[size_reg].val;
				}
			}

			if (insn_is_call(insn)
					&& insn->operand.rel == (unsigned long)vm_object_check_array) {
				struct insn *insn_push_index, *insn_push_array;
				uint32_t index_reg, array_reg;

				insn_push_index = list_entry(insn->insn_list_node.prev->prev,
								struct insn, insn_list_node);
				index_reg = insn_push_index->src.reg.interval->var_info->vreg;

				insn_push_array = list_entry(insn->insn_list_node.prev,
								struct insn, insn_list_node);
				array_reg = insn_push_array->src.reg.interval->var_info->vreg;

				if (regs_value[index_reg].active && arrays_value[array_reg].active) {
					if (regs_value[index_reg].val < arrays_value[array_reg].size)
						delete[index++] = insn;
				}
			}
		}
	}

	for (int i = 0; i < index; i++) {
		struct insn *insn_push_index, *insn_push_array,
			*insn, *insn_args_cleanup,
			*insn_exception_test1, *insn_exception_test2;

		insn = delete[i];
		insn_push_index = list_entry(insn->insn_list_node.prev->prev,
					struct insn, insn_list_node);
		insn_push_array = list_entry(insn->insn_list_node.prev,
					struct insn, insn_list_node);
		insn_args_cleanup = list_entry(insn->insn_list_node.next,
					struct insn, insn_list_node);
		insn_exception_test1 = list_entry(insn->insn_list_node.next->next,
					struct insn, insn_list_node);
		insn_exception_test2 = list_entry(insn->insn_list_node.next->next->next,
					struct insn, insn_list_node);

		remove_insn(insn);
		remove_insn(insn_push_index);
		remove_insn(insn_push_array);
		remove_insn(insn_args_cleanup);
		remove_insn(insn_exception_test1);
		remove_insn(insn_exception_test2);
	}
}
