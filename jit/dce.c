/*
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
#include "lib/radix-tree.h"
#include "vm/stdlib.h"

static struct dce *alloc_dce_element(struct var_info *var)
{
	struct dce *dce_element;

	dce_element = malloc(sizeof(struct dce));
	if (!dce_element)
		return NULL;

	dce_element->var = var;
	dce_element->next = NULL;

	return dce_element;
}

static void insert_list(struct dce *dce_element, struct dce *list)
{
	if (list != NULL){
		dce_element->next = list;
		list = dce_element;
	} else
		list = dce_element;
}

/*
 * This function implements dead code elimination. For more
 * details on the algorithm used here, check "Modern Compiler
 * Implementation in Java" by Andrew W. Appel, section 19.3.
 */
int dce(struct compilation_unit *cu)
{
	struct live_interval *it;
	struct var_info *var;
	struct dce *dce_element, *list;
	struct use_position *use;
	struct insn *def_insn;
	bool used;

	list = NULL;

	for_each_variable(var, cu->ssa_var_infos) {
		it = var->interval;

		if (!interval_has_fixed_reg(it)) {
			dce_element = alloc_dce_element(var);
			insert_list(dce_element, list);
		}
	}

	while (list) {
		dce_element = list;

		it = dce_element->var->interval;

		used = false;
		def_insn = NULL;
		list_for_each_entry(use, &it->use_positions, use_pos_list) {
			if (insn_vreg_use(use, it->var_info)) {
				used = true;
				break;
			}
			if (insn_vreg_def(use, it->var_info)) {
				def_insn = use->insn;
			}
		}

		list = dce_element->next;
		free(dce_element);

		if (!used) {
			unsigned long nr_par, nr_uses;
			struct use_position **reg_uses;

			if (!def_insn)
				continue;

			nr_par = nr_srcs_phi(def_insn) + MAX_REG_OPERANDS;
			reg_uses = malloc(nr_par * sizeof(struct use_position *));
			if (!reg_uses)
				return warn("out of memory"), -EINVAL;

			nr_uses = insn_uses_reg(def_insn, reg_uses);
			for (unsigned long i = 0; i < nr_uses; i++) {
				struct use_position *this_use = reg_uses[i];

				if (!interval_has_fixed_reg(this_use->interval)) {
					dce_element =
					    alloc_dce_element(this_use->
							      interval->
							      var_info);

					insert_list(dce_element, list);
				}
			}

			free(reg_uses);

			list_del(&def_insn->insn_list_node);
			if (insn_use_def(def_insn))
				hash_map_remove(cu->insn_add_ons, def_insn);
			if (insn_is_phi(def_insn))
				free_ssa_insn(def_insn);
			else
				free_insn(def_insn);
		}
	}

	return 0;
}
