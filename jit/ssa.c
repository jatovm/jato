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
 * For more details on the algorithms used here, please refer to the following
 * paper:
 *
 *   Optimization in Static Single Assignment Form
 *   External Specification
 *   December 2007
 *   Sassa Laboratory
 *   Graduate School of Information Science and Engineering
 *   Tokyo Institute of Technology
 */

#include "jit/bc-offset-mapping.h"
#include "jit/compilation-unit.h"
#include "jit/compiler.h"
#include "jit/instruction.h"
#include "jit/ssa.h"
#include "jit/vars.h"
#include "lib/bitset.h"
#include "lib/radix-tree.h"
#include "vm/stdlib.h"
#include <stdio.h>

static void reg_final_process(const void *);

struct key_operations insn_add_ons_key = {
        .hash           = ptr_hash,
        .equals         = ptr_equals,
        .final_process  = reg_final_process
};


static void reg_final_process(const void *value)
{
	struct use_position *reg;

	reg = (struct use_position *)value;
	list_del(&reg->use_pos_list);
	free(reg);
}

static struct use_position *init_insn_add_ons_reg(struct insn *insn,
						struct var_info *var)
{
	struct use_position *reg;

	reg = malloc(sizeof(struct use_position));

	INIT_LIST_HEAD(&reg->use_pos_list);
	reg->insn = insn;
	reg->interval = var->interval;
	reg->kind = USE_KIND_INVALID;

	list_add(&reg->use_pos_list, &var->interval->use_positions);

	return reg;
}

void recompute_insn_positions(struct compilation_unit *cu)
{
	free_radix_tree(cu->lir_insn_map);
	compute_insn_positions(cu);
}

static int ssa_analyze_liveness(struct compilation_unit *cu)
{
	int err = 0;

	err = init_sets(cu);
	if (err)
		goto out;

	analyze_use_def(cu);

	err = analyze_live_sets(cu);
out:
	return err;
}

static struct changed_var_stack *alloc_changed_var_stack(unsigned long vreg)
{
	struct changed_var_stack *changed;

	changed = malloc(sizeof(struct changed_var_stack));
	if (!changed)
		return NULL;

	changed->vreg = vreg;
	INIT_LIST_HEAD(&changed->changed_var_stack_node);

	return changed;
}

static int list_changed_stacks_add(struct list_head *list_changed_stacks,
	unsigned long vreg)
{
	struct changed_var_stack *changed;

	changed = alloc_changed_var_stack(vreg);
	if (!changed)
		return -ENOMEM;
	list_add(&changed->changed_var_stack_node, list_changed_stacks);

	return 0;
}

/*
 * This function returns true if the basic block is part of
 * the exception handler control flow.
 */
static bool bb_is_eh(struct compilation_unit *cu, struct basic_block *bb)
{
	return !bb->dfn && cu->entry_bb != bb;
}

static void free_ssa_liveness(struct compilation_unit *cu)
{
	struct basic_block *bb;

	for_each_basic_block(bb, &cu->bb_list) {
		free(bb->def_set);
		free(bb->use_set);
		free(bb->live_in_set);
		free(bb->live_out_set);
	}
}

static void free_insn_add_ons(struct compilation_unit *cu)
{
	free_hash_map(cu->insn_add_ons);
}

static void free_positions_as_predecessor(struct compilation_unit *cu)
{
	struct basic_block *bb;

	for_each_basic_block(bb, &cu->bb_list) {
		if (bb_is_eh(cu, bb))
			continue;

		free(bb->positions_as_predecessor);
	}
}

static void free_ssa(struct compilation_unit *cu)
{
	struct basic_block *bb;

	for_each_basic_block(bb, &cu->bb_list) {
		free(bb->def_set);
                free(bb->use_set);
                free(bb->live_in_set);
                free(bb->live_out_set);

		if (bb_is_eh(cu, bb))
			continue;

		free(bb->positions_as_predecessor);

		free(bb->dom_successors);
	}

	free_insn_add_ons(cu);
}

static int compute_dom_successors(struct compilation_unit *cu)
{
	struct basic_block *bb, *dom;
	unsigned long *positions, idx;

	positions = malloc(nr_bblocks(cu) * sizeof(long));
	if (!positions)
		return warn("out of memory"), -ENOMEM;

	for_each_basic_block(bb, &cu->bb_list) {
		if (!bb->dfn)
			continue;

		dom = cu->doms[bb->dfn];

		if (dom)
			dom->nr_dom_successors++;
	}

	idx = 0;
	for_each_basic_block(bb, &cu->bb_list) {
		positions[idx++] = 0;

		if (bb_is_eh(cu, bb)
				|| bb->nr_dom_successors == 0)
			continue;

		bb->dom_successors = malloc(bb->nr_dom_successors * sizeof(struct basic_block*));
		if (!bb->dom_successors) {
			free(positions);
			return warn("out of memory"), -ENOMEM;
		}
	}

	for_each_basic_block(bb, &cu->bb_list){
		if (!bb->dfn)
			continue;

		dom = cu->doms[bb->dfn];

		if (dom)
			dom->dom_successors[positions[dom->dfn]++] = bb;
	}

	free(positions);

	return 0;
}

static int compute_positions_as_predecessor(struct compilation_unit *cu)
{
	struct basic_block *bb;

	for_each_basic_block(bb, &cu->bb_list) {
		if (bb_is_eh(cu, bb))
			continue;

		bb->positions_as_predecessor = malloc(bb->nr_successors * sizeof(unsigned long));
		if (!bb->positions_as_predecessor)
			return warn("out of memory"), -ENOMEM;

		for (unsigned long i = 0; i < bb->nr_successors; i++) {
			struct basic_block *succ = bb->successors[i];
			unsigned long position;

			position = 0;
			for (unsigned long j = 0; j < succ->nr_predecessors; j++) {
				struct basic_block *pred = succ->predecessors[j];
				if (bb_is_eh(cu, pred))
					continue;

				if (pred == bb) {
					bb->positions_as_predecessor[i] = position;
					break;
				}

				position++;
			}
		}
	}

	return 0;
}

static void compute_nr_predecessors_no_eh(struct compilation_unit *cu)
{
	struct basic_block *bb;
	unsigned long i;

	for_each_basic_block(bb, &cu->bb_list) {
		if (bb_is_eh(cu, bb))
			continue;

		for (i = 0; i < bb->nr_predecessors; i++) {
			if (bb_is_eh(cu, bb->predecessors[i]))
				continue;
			else bb->nr_predecessors_no_eh++;
		}
	}
}

static void change_operand_var(struct use_position *reg, struct var_info *new_var)
{
	list_add(&reg->use_pos_list, &new_var->interval->use_positions);
	reg->interval = new_var->interval;
}

static void insert_phi(struct basic_block *bb, struct var_info *var)
{
	struct insn *insn;

	insn = ssa_phi_insn(var, bb->nr_predecessors_no_eh);
	bb_add_first_insn(bb, insn);
}

static int do_insn_is_copy(struct use_position *reg,
		struct use_position **regs_uses,
		struct list_head *list_changed_stacks,
		struct stack **name_stack)
{
	struct var_info *var;

	var = reg->interval->var_info;

	stack_push(name_stack[var->vreg], (*regs_uses)->interval->var_info);

	return list_changed_stacks_add(list_changed_stacks, var->vreg);
}

static int add_var_info(struct compilation_unit *cu,
	struct use_position *reg,
	struct list_head *list_changed_stacks,
	struct stack **name_stack)
{
	struct live_interval *it;
	struct var_info *var, *new_var;
	int err;

	it = reg->interval;
	var = it->var_info;

	if (interval_has_fixed_reg(it))
		new_var = ssa_get_fixed_var(cu, var->interval->reg);
	else
		new_var = ssa_get_var(cu, var->vm_type);

	stack_push(name_stack[var->vreg], new_var);

	err = list_changed_stacks_add(list_changed_stacks, var->vreg);
	if (err)
		return err;

	change_operand_var(reg, new_var);

	return 0;
}

static void replace_parameter_var_info(struct use_position *reg,
				struct stack **name_stack)
{
	struct live_interval *it;
	struct var_info *var, *new_var;

	it = reg->interval;
	var = it->var_info;

	new_var = stack_peek(name_stack[var->vreg]);

	change_operand_var(reg, new_var);
}

static void eh_replace_var_info(struct compilation_unit *cu,
			struct use_position *reg,
			struct stack **name_stack,
			struct insn *insn)
{
	struct live_interval *it;
	struct var_info *var, *new_var;

	it = reg->interval;
	var = it->var_info;

	if (stack_is_empty(name_stack[var->vreg])){
		if (interval_has_fixed_reg(it))
			new_var = ssa_get_fixed_var(cu, var->interval->reg);
		else
			new_var = ssa_get_var(cu, var->vm_type);

		stack_push(name_stack[var->vreg], new_var);
	}
	else
		new_var = stack_peek(name_stack[var->vreg]);

	change_operand_var(reg, new_var);
}

static int replace_var_info(struct compilation_unit *cu,
			struct use_position *reg,
			struct list_head *list_changed_stacks,
			struct stack **name_stack,
			struct insn *insn)
{
	struct live_interval *it;
	struct var_info *var, *new_var;

	it = reg->interval;
	var = it->var_info;

	if (stack_is_empty(name_stack[var->vreg]))
		return warn("Variable %lu not initialized for ssa renaming", var->vreg), -EINVAL;

	new_var = stack_peek(name_stack[var->vreg]);

	/*
	 * If the instruction uses and defines the same register, then we
	 * keep track of the used register in insn_add_ons.
	 */
	if (!((insn_use_def_src(insn) && &insn->src.reg == reg )
                        || (insn_use_def_dst(insn) && &insn->dest.reg == reg)))
		change_operand_var(reg, new_var);
	else {
		struct use_position *new_reg;

		new_reg = init_insn_add_ons_reg(insn, new_var);
		hash_map_put(cu->insn_add_ons, insn, new_reg);
	}

	return 0;
}

static int insert_stack_fixed_var(struct compilation_unit *cu,
				struct stack **name_stack,
				struct list_head *list_changed_stacks)
{
	struct var_info *var, *new_var;
	int err;

	for_each_variable(var, cu->var_infos) {
		if (interval_has_fixed_reg(var->interval)) {
			new_var = ssa_get_fixed_var(cu, var->interval->reg);

			stack_push(name_stack[var->vreg], new_var);

			err = list_changed_stacks_add(list_changed_stacks, var->vreg);
			if (err)
				return err;
		}
	}

	return 0;
}

static int __rename_variables(struct compilation_unit *cu,
			struct basic_block *bb,
			struct stack **name_stack)
{
	struct insn *insn, *insn_next;
	struct use_position *regs_uses[MAX_REG_OPERANDS],
				*regs_defs[MAX_REG_OPERANDS + 1];
	struct use_position *reg;
	struct list_head *list_changed_stacks;
	struct changed_var_stack *changed, *this, *next;
	int nr_defs, nr_uses, i, err;
	bool delete;

	list_changed_stacks = malloc(sizeof (struct list_head));
	if (!list_changed_stacks)
		return warn("out of memory"), -ENOMEM;

	INIT_LIST_HEAD(list_changed_stacks);

	if (bb == cu->entry_bb) {
		err = insert_stack_fixed_var(cu, name_stack, list_changed_stacks);
		if (err)
			return err;
	}

	list_for_each_entry_safe(insn, insn_next, &bb->insn_list, insn_list_node) {
		if (!insn_is_phi(insn)) {
			nr_uses = insn_uses_reg(insn, regs_uses);
			for (i = 0; i < nr_uses; i++) {
				reg = regs_uses[i];

				err = replace_var_info(cu, reg, list_changed_stacks, name_stack, insn);
				if (err)
					return err;
			}
		}

		delete = false;
		nr_defs = insn_defs_reg(insn, regs_defs);
		for (i = 0; i < nr_defs; i++) {
			reg = regs_defs[i];

			/*
			 * We do copy folding simultaneously with th conversion to SSA form.
			 * For more details about copy folding during LIR->SSA conversion,
			 * see section 4.2.2 of "Optimizations in Static Single Assignment Form,
			 * Sassa Laboratory"
			 */
			if (insn_is_copy(insn) && !interval_has_fixed_reg(reg->interval)
						&& !interval_has_fixed_reg((*regs_uses)->interval)) {
				delete = true;

				err = do_insn_is_copy(reg, regs_uses, list_changed_stacks, name_stack);
				if (err)
					return err;
			} else {
				err = add_var_info(cu, reg, list_changed_stacks, name_stack);
				if (err)
					return err;
			}
		}

		if (delete) {
			list_del(&insn->insn_list_node);
			free_insn(insn);
		}
	}

	/*
	 * Update parameters of phi functions in succesors list
	 */
	for (unsigned long i = 0; i < bb->nr_successors; i++) {
		struct basic_block *succ = bb->successors[i];

		for_each_insn(insn, &succ->insn_list){
			if (!insn_is_phi(insn))
				break;

			reg = &insn->ssa_srcs[bb->positions_as_predecessor[i]].reg;
			replace_parameter_var_info(reg, name_stack);
		}
	}

	/*
	 * Rename along the dominator tree from the top to
	 * the bottom.
	 */
	for (unsigned long i = 0; i < bb->nr_dom_successors; i++) {
		struct basic_block *dom_succ = bb->dom_successors[i];

		__rename_variables(cu, dom_succ, name_stack);
	}

	/*
	 * Pop all the variables that have been put onto the stack
	 * during the processing of block X.
	 */
	list_for_each_entry(changed, list_changed_stacks, changed_var_stack_node)
		stack_pop(name_stack[changed->vreg]);

	list_for_each_entry_safe(this, next, list_changed_stacks, changed_var_stack_node)
		free(this);

	free(list_changed_stacks);

	return 0;
}

static void eh_rename_variables(struct compilation_unit *cu,
				struct basic_block *bb,
				struct stack **name_stack)
{
	struct insn *insn;
	struct use_position *reg;
	struct use_position *regs_uses[MAX_REG_OPERANDS],
			*regs_defs[MAX_REG_OPERANDS + 1];
	int nr_defs, nr_uses, i;

	for_each_insn(insn, &bb->insn_list) {
		nr_uses = insn_uses_reg(insn, regs_uses);
		for (i = 0; i < nr_uses; i++) {
			reg = regs_uses[i];

			if (insn_use_def_src(insn) && &insn->src.reg == reg)
				continue;

			if (insn_use_def_dst(insn) && &insn->dest.reg == reg)
				continue;

			eh_replace_var_info(cu, reg, name_stack, insn);
		}

		nr_defs = insn_defs_reg(insn, regs_defs);
		for (i = 0; i < nr_defs; i++) {
			reg = regs_defs[i];

			eh_replace_var_info(cu, reg, name_stack, insn);
		}
	}

	bb->is_renamed = true;

	for (unsigned int i = 0; i < bb->nr_successors; i++)
		if (bb_is_eh(cu, bb->successors[i]) && !bb->successors[i]->is_renamed) {
			eh_rename_variables(cu, bb->successors[i], name_stack);
		}
}

/*
 * See section 4.2 "Renaming of variables" of
 * "Optimizations in Static Single Assignment Form, Sassa Laboratory"
 * for details of the implementation.
 */
static int rename_variables(struct compilation_unit *cu)
{
	struct stack *name_stack[cu->nr_vregs];
	struct basic_block *bb;
	int err;

	for (unsigned long i = 0; i < cu->nr_vregs; i++)
		name_stack[i] = alloc_stack();

	err = __rename_variables(cu, cu->entry_bb, name_stack);
	if (err)
		return err;

	/*
	 * Virtual registers need to be renamed even in exception
	 * handler basic blocks because, otherwise, there would
	 * be different virtual registers with the same vreg value
	 * and the register allocator would issue incorrect results.
	 * Exception handler basic blocks are still in non-SSA
	 * form because we just rename the variables.
	 */
	for_each_basic_block(bb, &cu->bb_list) {
		if (bb->is_eh && !bb->is_renamed)
			eh_rename_variables(cu, bb, name_stack);
	}

	for (unsigned long i = 0; i < cu->nr_vregs; i++)
		free_stack(name_stack[i]);

	return 0;
}

static void iterate_dom_frontier_set(struct compilation_unit *cu,
				struct var_info *var,
				struct basic_block *bb,
				struct bitset *workset,
				unsigned long *inserted,
				unsigned long *work,
				int *last_ndx)
{
	struct bitset *temp_dom_frontier;
	struct basic_block *bb_ndx;
	int ndx;

	temp_dom_frontier = alloc_bitset(nr_bblocks(cu));

	bitset_copy_to(bb->dom_frontier, temp_dom_frontier);

	ndx = 0;
	while ((ndx = bitset_ffs_from(temp_dom_frontier, ndx)) != -1) {
		clear_bit(temp_dom_frontier->bits, ndx);

		bb_ndx = cu->bb_df_array[ndx];
		if (inserted[ndx] != var->vreg && test_bit(bb_ndx->live_in_set->bits, var->vreg)) {
			inserted[ndx] = var->vreg;
			insert_phi(bb_ndx, var);

			if (work[ndx] != var->vreg) {
				set_bit(workset->bits, ndx);
				work[ndx] = var->vreg;
				if (ndx < *last_ndx)
					*last_ndx = ndx;
			}
		}
	}

	free(temp_dom_frontier);

}

/*
 * See section 4.1 "Inserting phi-functions" of
 * "Optimizations in Static Single Assignment Form, Sassa Laboratory"
 * for details of the implementation.
 */
static int insert_phi_insns(struct compilation_unit *cu)
{
	struct bitset *workset;
	struct basic_block *bb;
	unsigned long *inserted, *work;
	int ndx;
	struct var_info *var;

	inserted = malloc(nr_bblocks(cu) * sizeof(unsigned long));
	if (!inserted)
		return warn("out of memory"), -ENOMEM;

	work = malloc(nr_bblocks(cu) * sizeof(unsigned long));
	if (!work)
		return warn("out of memory"), -ENOMEM;

	for_each_basic_block(bb, &cu->bb_list) {
		/* skip exception handler basic blocks */
		if (bb_is_eh(cu, bb))
			continue;

		work[bb->dfn] = SSA_INIT_BLOCK;
		inserted[bb->dfn] = SSA_INIT_BLOCK;
	}

	workset = alloc_bitset(nr_bblocks(cu));

	for_each_variable(var, cu->var_infos) {
		ndx = -1;
		for_each_basic_block(bb, &cu->bb_list) {
			if (bb_is_eh(cu, bb))
				continue;

			if (test_bit(bb->def_set->bits, var->vreg)) {
				set_bit(workset->bits, bb->dfn);
				work[bb->dfn] = var->vreg;

				if (ndx == -1)
					ndx = bb->dfn;
				else if (ndx > bb->dfn)
					ndx = bb->dfn;
			}
		}

		if (ndx == -1)
			continue;

		while ((ndx = bitset_ffs_from(workset, ndx)) != -1) {
			clear_bit(workset->bits, ndx);

			bb = cu->bb_df_array[ndx];

			iterate_dom_frontier_set(cu, var, bb, workset, inserted, work, &ndx);
		}
	}

	free(workset);
	free(inserted);
	free(work);

	return 0;
}

static int __insert_instruction_pass(struct compilation_unit *cu, struct basic_block *bb)
{
	struct insn *insn, *new_insn;
	struct var_info *var1, *var2;
	struct use_position *reg;

	for_each_insn(insn, &bb->insn_list) {
		if (insn_use_def_src(insn)) {
			var1 = insn->src.reg.interval->var_info;
		} else if (insn_use_def_dst(insn)) {
			var1 = insn->dest.reg.interval->var_info;
		} else continue;

		hash_map_get(cu->insn_add_ons, insn, (void **) &reg);
		if (!reg)
			return warn("no entry in hashtable for insn %d\n", insn->type), -EINVAL;

		var2 = reg->interval->var_info;

		if (interval_has_fixed_reg(var2->interval)
				&& interval_has_fixed_reg(var1->interval))
			continue;

		new_insn = ssa_reg_reg_insn(var2, var1);
		insn_set_bc_offset(new_insn, insn->bc_offset);
		list_add(&new_insn->insn_list_node, insn->insn_list_node.prev);
	}

	return 0;
}
/*
 * This function adds mov_reg_reg instructions before instructions
 * that use and define the same register, so that the virtual register
 * that is defined in the original instruction has the correct use value.
 * Example:
 * 	In SSA form we may have "insn_add_imm_reg $0x4, r18" which uses
 *	and defines r18, but being in SSA form, r18 appears for the first
 *	time in code. We keep the virtual register that is being used in
 *	the instruction in insn_add_ons, and during SSA deconstruction
 *	we move the virtual register form insn_add_ons into r18, so that
 *	r18 = r18 + 4 issues the correct result.
 *
 *	We don't insert mov_reg_reg instructions for fixed virtual
 *	registers because, for example, "mov %ebx, %ebx" is redundant.
 */
static void insert_instruction_pass(struct compilation_unit *cu)
{
	struct basic_block *bb;

	for_each_basic_block(bb, &cu->bb_list) {
		if (bb_is_eh(cu, bb))
			continue;

		__insert_instruction_pass(cu, bb);
	}
}

/*
 * Converting out of SSA form requires the introduction of moves into the pre-
 * decessor blocks of phi instructions. According to section 6.1 in "Constructing
 * SSA the Easy Way, Michael Bebenita", empty basic blocks have to be inserted
 * to split critical edges. We have also added an empty basic block if the predecessor
 * basic block ends with a branch instruction.
 */
static struct basic_block *det_insertion_bb(struct compilation_unit *cu,
					struct basic_block *pred_bb,
					struct basic_block *bb,
					unsigned int bc_offset)
{
	struct insn *last_insn;
	struct basic_block *r_bb;

	last_insn = bb_last_insn(pred_bb);

	r_bb = pred_bb;

	if (pred_bb->nr_successors > 1) {
		if (insn_is_jmp_mem(last_insn))
			/*
			 * In case we have a predecessor basic block ending in
			 * in an INSN_JMP_MEMBASE or INSN_JMP_MEMINDEX instruction,
			 * then we add the mov_reg_reg instructions in bb and move
			 * bb contents into the newly created basic block (that
			 * is bb's successor) because the instruction jumps to bb.
			 */
			r_bb = ssa_insert_chg_bb(cu, pred_bb, bb, bc_offset);
		else {
			struct basic_block *after_bb;
			struct list_head *after_bb_node;

			after_bb_node = pred_bb->bb_list_node.next;
			after_bb = container_of(after_bb_node, struct basic_block, bb_list_node);

			r_bb = ssa_insert_empty_bb(cu, pred_bb, bb, bc_offset);

			if (last_insn)
				ssa_chg_jmp_direction(last_insn, after_bb, r_bb, bb);
		}
	}

	return r_bb;
}

static void insert_insn(struct basic_block *bb,
			struct var_info *src,
			struct var_info *dest,
			unsigned int bc_offset)
{
	struct insn *new_insn, *last_insn;

	if (interval_has_fixed_reg(dest->interval)
			&& interval_has_fixed_reg(src->interval))
		return;

	new_insn = ssa_reg_reg_insn(src, dest);
	insn_set_bc_offset(new_insn, bc_offset);

	last_insn = bb_last_insn(bb);
	if (insn_is_jmp_branch(last_insn))
		list_add(&new_insn->insn_list_node, last_insn->insn_list_node.prev);
	else
		list_add_tail(&new_insn->insn_list_node, &bb->insn_list);
}

static int insert_copy_insns(struct compilation_unit *cu,
				struct basic_block *pred_bb,
				struct basic_block **bb,
				unsigned int bc_offset,
				int phi_arg)
{
	struct insn *insn, *last_insn;
	struct basic_block *insertion_bb, *aux_bb;
	struct insn *jump;

	insertion_bb = det_insertion_bb(cu, pred_bb, *bb, bc_offset);
	if (!insertion_bb)
		return warn("Out of memory"), -ENOMEM;

	last_insn = bb_last_insn(pred_bb);
	if (insn_is_jmp_mem(last_insn)) {
		aux_bb = *bb;
		*bb = insertion_bb;
		insertion_bb = aux_bb;
	}

	for_each_insn(insn, &(*bb)->insn_list) {
		if (!insn_is_phi(insn))
			break;

		if (interval_has_fixed_reg(insn->ssa_dest.reg.interval))
			continue;

		insert_insn(insertion_bb, insn->ssa_srcs[phi_arg].reg.interval->var_info,
				insn->ssa_dest.reg.interval->var_info, insertion_bb->end);
	}

	last_insn = bb_last_insn(insertion_bb);
	if (!insn_is_jmp_branch(last_insn)) {
		jump = jump_insn(*bb);
		insn_set_bc_offset(jump, insertion_bb->end);
		list_add_tail(&jump->insn_list_node, &insertion_bb->insn_list);
	}

	return 0;
}

static int __ssa_deconstruction(struct compilation_unit *cu,
				struct basic_block *bb)
{
	struct insn *insn, *tmp, *last_insn;
	int phi_arg, err;
	unsigned int bc_offset;

	/*
	 * We do not insert copy instructions for fixed virtual registers
	 * because they would be redundant (Example: mov r10=EAX, r11=EAX). If
	 * we have a basic block containing only phi instructions for fixed
	 * virtual registers, then no deconstruction needs to be done.
	 */
	for_each_insn(insn, &bb->insn_list) {
		if (!insn_is_phi(insn))
			goto out;

		if (!interval_has_fixed_reg(insn->ssa_dest.reg.interval))
			break;
	}

	phi_arg = 0;

	for (unsigned long i = 0; i < bb->nr_predecessors; i++) {
		struct basic_block *pred_bb;

		pred_bb = bb->predecessors[i];
		if (bb_is_eh(cu, pred_bb))
			continue;

		last_insn = bb_last_insn(pred_bb);
		if (last_insn)
			bc_offset = last_insn->bc_offset;
		else
			bc_offset = 0;

		err = insert_copy_insns(cu, pred_bb, &bb, bc_offset, phi_arg);

		if (err)
			return err;

		phi_arg++;
	}

out:
	list_for_each_entry_safe(insn, tmp, &bb->insn_list, insn_list_node) {
		if (!insn_is_phi(insn))
			break;

		list_del(&insn->insn_list_node);
		free_ssa_insn(insn);
	}

	for (unsigned long i = 0; i < bb->nr_dom_successors; i++) {
		__ssa_deconstruction(cu, bb->dom_successors[i]);
	}

	return 0;
}

/*
 * The SSA deconstruction only works if there are not cyclic
 * dependencies between phi instructions in the same basic block
 * (this cycles can appear after some optimizations, like copy
 * propagation for example).
 */
static int ssa_deconstruction(struct compilation_unit *cu)
{
	int err;

	err = __ssa_deconstruction(cu, cu->entry_bb);
	if (err)
		return err;

	return 0;
}

static void replace_var_infos(struct compilation_unit *cu)
{
	cu->var_infos = cu->ssa_var_infos;
	cu->nr_vregs = cu->ssa_nr_vregs;

	for (int i = 0; i < NR_FIXED_REGISTERS; i++)
		cu->fixed_var_infos[i] = NULL;
}

static int init_ssa(struct compilation_unit *cu)
{
	int err;

	compute_nr_predecessors_no_eh(cu);

	err = compute_positions_as_predecessor(cu);
	if (err)
		goto error_positions;

	err = compute_dom_successors(cu);
	if (err)
		goto error_dom;

	cu->insn_add_ons = alloc_hash_map_with_size(32, &insn_add_ons_key);

	return 0;

error_dom:
	free_positions_as_predecessor(cu);

error_positions:
	return err;
}

/*
 * This function eliminates an instruction in the following
 * 4 situations:
 *
 ********
 * mov_imm_reg			x, r10
 *					=> mov_imm_membase		x, y
 * mov_reg_membase		r10, y
 ********
 * mov_imm_reg			x, r10
 *					=> mov_imm_reg			x, r11
 * mov_reg_reg			r10, r11
 ********
 * mov_imm_reg			x, r10
 *					=> mov_imm_memlocal		x, y
 * mov_reg_memlocal		r10, y
 ********
 * mov_imm_reg			x, r10
 *					=> mov_imm_thread_local_membase	x, y
 * mov_reg_therad_local_membase	r10, y
 ********
 *
 */
void imm_copy_propagation(struct compilation_unit *cu)
{
	struct basic_block *bb;
	struct insn *insn, *tmp;

	for_each_basic_block(bb, &cu->bb_list) {
		list_for_each_entry_safe(insn, tmp, &bb->insn_list, insn_list_node) {
			if (insn_is_mov_imm_reg(insn)) {
				struct operand *operand = &insn->dest;

				struct use_position *reg = &operand->reg;

				if (!interval_has_fixed_reg(reg->interval)) {
					struct insn *rep_insn = NULL;
					struct use_position *use;
					int cnt = 0;

					list_for_each_entry(use, &reg->interval->use_positions, use_pos_list) {
						cnt++;
						if (cnt == 3)
							break;

						if (!insn_is_mov_imm_reg(use->insn))
							rep_insn = use->insn;
					}

					if (cnt == 2) {
						if (ssa_modify_insn_type(rep_insn))
							continue;

						list_del(&rep_insn->src.reg.use_pos_list);

						imm_operand(&rep_insn->src, insn->src.imm);

						list_del(&insn->insn_list_node);
						free_insn(insn);
					}
				}
			}
		}
	}
}

int lir_to_ssa(struct compilation_unit *cu)
{
	int err;

	err = init_ssa(cu);
	if (err)
		goto error;

	err = ssa_analyze_liveness(cu);
	if (err)
		goto error;

	err = insert_phi_insns(cu);
	if (err)
		goto error_def;;

	cu->ssa_nr_vregs = NR_FIXED_REGISTERS;

	err = rename_variables(cu);
	if (err)
		goto error_def;

	return 0;

error_def:
	free_ssa_liveness(cu);
error:
	return err;
}

/*
 * This function creates the compilation unit's fixed_var_infos
 * array with empty intervals because after the register allocation
 * phase (that does not permit any more var_info allocations) there can
 * still be requests for fixed var_infos (for example, convert_ic_calls
 * function).
 */
static void create_fixed_var_infos(struct compilation_unit *cu)
{
	for (int ndx = 0; ndx < NR_FIXED_REGISTERS; ndx++) {
		get_fixed_var(cu, ndx);
	}
}

static void cumulate_fixed_var_infos(struct compilation_unit *cu)
{
	struct var_info *var, *prev;
	struct live_interval *it;
	struct use_position *this, *next;

	prev = NULL;
	var = cu->var_infos;

	while (var != NULL) {
		it = var->interval;

		if (interval_has_fixed_reg(it)
				&& cu->fixed_var_infos[it->reg] != var) {
			struct var_info *fixed_var = cu->fixed_var_infos[it->reg];
			struct live_interval *fixed_it = fixed_var->interval;

			list_for_each_entry_safe(this, next, &it->use_positions, use_pos_list) {
				list_add_tail(&this->use_pos_list, &fixed_it->use_positions);
				this->interval = fixed_var->interval;
			}

			if (prev == NULL) {
				cu->var_infos = cu->var_infos->next;
			} else {
				prev->next = var->next;
			}
		} else
			prev = var;

		var = var->next;
	}
}

int ssa_to_lir(struct compilation_unit *cu)
{
	int err;

	insert_instruction_pass(cu);

	err = ssa_deconstruction(cu);
	if (err)
		return err;

	recompute_insn_positions(cu);

	replace_var_infos(cu);
	free_ssa(cu);

	create_fixed_var_infos(cu);
	cumulate_fixed_var_infos(cu);

	return 0;
}
