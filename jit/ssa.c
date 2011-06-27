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

static void recompute_insn_positions(struct compilation_unit *cu)
{
	free_radix_tree(cu->lir_insn_map);
	compute_insn_positions(cu);
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

static void analyze_bb_def(struct basic_block *bb, struct insn *insn)
{
	struct var_info *defs[MAX_REG_OPERANDS];
	int nr_defs;
	int i;

	nr_defs = insn_defs(bb->b_parent, insn, defs);
	for (i = 0; i < nr_defs; i++) {
		struct var_info *var = defs[i];

		set_bit(bb->def_set->bits, var->vreg);
	}
}

static int analyze_def(struct compilation_unit *cu)
{
	struct basic_block *bb;

	for_each_basic_block(bb, &cu->bb_list) {
		struct insn *insn;

		bb->def_set = alloc_bitset(cu->nr_vregs);
		if (!bb->def_set)
			return warn("out of memory"), -ENOMEM;

		for_each_insn(insn, &bb->insn_list) {
			analyze_bb_def(bb, insn);
		}
	}

	return 0;
}

/*
 * This function returns true if the basic block is part of
 * the exception handler control flow.
 */
bool bb_is_eh(struct compilation_unit *cu, struct basic_block *bb)
{
	return !bb->dfn && cu->entry_bb != bb;
}

static void free_def_set(struct compilation_unit *cu)
{
	struct basic_block *bb;

	for_each_basic_block(bb, &cu->bb_list) {
		free(bb->def_set);
	}
}

static void free_insn_add_ons(struct compilation_unit *cu)
{
	struct insn_add_ons *insn_add_ons, *tmp;
	struct basic_block *bb;

	for_each_basic_block(bb, &cu->bb_list) {
		if (bb_is_eh(cu, bb))
			continue;

		list_for_each_entry_safe(insn_add_ons, tmp,
				&bb->insn_add_ons_list, insn_list_node)
			free(insn_add_ons);
	}
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

static void free_dom_successors(struct compilation_unit *cu)
{
	struct basic_block *bb;

	for_each_basic_block(bb, &cu->bb_list) {
		if (bb_is_eh(cu, bb))
			continue;

		free(bb->dom_successors);
	}
}

static void free_ssa(struct compilation_unit *cu)
{
	struct basic_block *bb;
	struct insn_add_ons *insn_add_ons, *tmp;

	for_each_basic_block(bb, &cu->bb_list) {
		free(bb->def_set);

		if (bb_is_eh(cu, bb))
			continue;

		list_for_each_entry_safe(insn_add_ons, tmp,
				&bb->insn_add_ons_list, insn_list_node)
			free(insn_add_ons);

		free(bb->positions_as_predecessor);

		free(bb->dom_successors);
	}
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
			struct insn *insn,
			struct insn_add_ons *insn_add_ons)
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
	else
		insn_add_ons->var = new_var;

	return 0;
}

static void insert_def_insn(struct compilation_unit *cu,
			struct var_info *var,
			struct var_info *gpr,
			struct insn **conv_insn)
{
	struct insn *insn;

	insn = ssa_imm_reg_insn(INIT_VAL, var, gpr, conv_insn);
	if (!*conv_insn) {
		insn_set_bc_offset(insn, INIT_BC_OFFSET);

		bb_add_first_insn(cu->entry_bb, insn);
	} else {
		insn_set_bc_offset(insn, INIT_BC_OFFSET);
		insn_set_bc_offset(*conv_insn, INIT_BC_OFFSET);

		bb_add_first_insn(cu->entry_bb, insn);
		list_add(&(*conv_insn)->insn_list_node, &insn->insn_list_node);
	}
}

/*
 * The SSA form assumes that all variables have been initialized in
 * in the entry basic block of the CFG. That is why we add
 * initialization instructions for each non-fixed virtual register.
 */
static void insert_init_insn(struct compilation_unit *cu, struct var_info *gpr)
{
	struct var_info *var;
	struct insn *conv_insn;

	/*
	 * Values are initialized to 0.
	 */
	for_each_variable(var, cu->var_infos) {
		if (!interval_has_fixed_reg(var->interval)) {
			insert_def_insn(cu, var, gpr, &conv_insn);
		}
	}
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
	struct insn *insn;
	struct use_position *regs_uses[MAX_REG_OPERANDS],
				*regs_defs[MAX_REG_OPERANDS + 1];
	struct use_position *reg;
	struct insn_add_ons *insn_add_ons;
	struct list_head *insn_add_ons_list;
	struct list_head *list_changed_stacks;
	struct changed_var_stack *changed, *this, *next;
	int nr_defs, nr_uses, i, err;

	list_changed_stacks = malloc(sizeof (struct list_head));
	if (!list_changed_stacks)
		return warn("out of memory"), -ENOMEM;

	INIT_LIST_HEAD(list_changed_stacks);

	if (bb == cu->entry_bb) {
		err = insert_stack_fixed_var(cu, name_stack, list_changed_stacks);
		if (err)
			return err;
	}

	insn_add_ons_list = &bb->insn_add_ons_list;

	for_each_insn(insn, &bb->insn_list) {
		insn_add_ons_list = insn_add_ons_list->next;
		if (insn->type != INSN_PHI) {
			nr_uses = insn_uses_reg(insn, regs_uses);
			for (i = 0; i < nr_uses; i++) {
				reg = regs_uses[i];

				insn_add_ons = list_entry(insn_add_ons_list, struct insn_add_ons, insn_list_node);
				err = replace_var_info(cu, reg, list_changed_stacks, name_stack, insn, insn_add_ons);
				if (err)
					return err;
			}
		}

		nr_defs = insn_defs_reg(insn, regs_defs);
		for (i = 0; i < nr_defs; i++) {
			reg = regs_defs[i];

			err = add_var_info(cu, reg, list_changed_stacks, name_stack);
			if (err)
				return err;
		}
	}

	/*
	 * Update parameters of phi functions in succesors list
	 */
	for (unsigned long i = 0; i < bb->nr_successors; i++) {
		struct basic_block *succ = bb->successors[i];

		for_each_insn(insn, &succ->insn_list){
			if (insn->type != INSN_PHI)
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

	for (unsigned int i = 0; i < bb->nr_successors; i++)
		if (bb_is_eh(cu, bb->successors[i])) {
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
		if (bb->is_eh)
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
				unsigned long *work)
{
	struct bitset *temp_dom_frontier;
	int ndx;

	temp_dom_frontier = alloc_bitset(nr_bblocks(cu));

	bitset_copy_to(bb->dom_frontier, temp_dom_frontier);

	while ((ndx = bitset_ffs(temp_dom_frontier)) != -1) {
		clear_bit(temp_dom_frontier->bits, ndx);

		if (inserted[ndx] != var->vreg) {
			inserted[ndx] = var->vreg;
			insert_phi(cu->bb_df_array[ndx], var);

			if (work[ndx] != var->vreg) {
				set_bit(workset->bits, ndx);
				work[ndx] = var->vreg;
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
		for_each_basic_block(bb, &cu->bb_list) {
			if (bb_is_eh(cu, bb))
				continue;

			if (test_bit(bb->def_set->bits, var->vreg)) {
				set_bit(workset->bits, bb->dfn);
				work[bb->dfn] = var->vreg;
			}
		}

		while ((ndx = bitset_ffs(workset)) != -1) {
			clear_bit(workset->bits, ndx);

			bb = cu->bb_df_array[ndx];

			iterate_dom_frontier_set(cu, var, bb, workset, inserted, work);
		}
	}

	free(workset);
	free(inserted);
	free(work);

	return 0;
}

static void __insert_instruction_pass(struct basic_block *bb)
{
	struct insn *insn, *new_insn;
	struct var_info *var1, *var2;
	struct list_head *insn_add_ons_list;
	struct insn_add_ons *insn_add_ons;

	insn_add_ons_list = &bb->insn_add_ons_list;

	for_each_insn(insn, &bb->insn_list) {
		if (!(insn->flags & INSN_FLAG_SSA_ADDED)) {
			insn_add_ons_list = insn_add_ons_list->next;
		}

		if (insn_use_def_src(insn)) {
			var1 = insn->src.reg.interval->var_info;
		} else if (insn_use_def_dst(insn)) {
			var1 = insn->dest.reg.interval->var_info;
		} else continue;

		insn_add_ons = list_entry(insn_add_ons_list, struct insn_add_ons, insn_list_node);
		var2 = insn_add_ons->var;

		if (interval_has_fixed_reg(var2->interval)
				&& interval_has_fixed_reg(var1->interval))
			continue;

		new_insn = ssa_reg_reg_insn(var2, var1);
		insn_set_bc_offset(new_insn, insn->bc_offset);
		list_add(&new_insn->insn_list_node, insn->insn_list_node.prev);
		new_insn->flags |= INSN_FLAG_SSA_ADDED;
	}
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

		__insert_instruction_pass(bb);
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

	if (pred_bb->nr_successors > 1
			|| (last_insn && insn_is_branch(last_insn))) {
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
	struct insn *new_insn;

	if (interval_has_fixed_reg(dest->interval)
			&& interval_has_fixed_reg(src->interval))
		return;

	new_insn = ssa_reg_reg_insn(src, dest);
	insn_set_bc_offset(new_insn, bc_offset);
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
		if (insn->type != INSN_PHI)
			break;

		insert_insn(insertion_bb, insn->ssa_srcs[phi_arg].reg.interval->var_info,
				insn->ssa_dest.reg.interval->var_info, insertion_bb->end);
	}

	jump = jump_insn(*bb);
	insn_set_bc_offset(jump, insertion_bb->end);
	list_add_tail(&jump->insn_list_node, &insertion_bb->insn_list);

	return 0;
}

static int __ssa_deconstruction(struct compilation_unit *cu,
				struct basic_block *bb)
{
	struct insn *insn, *tmp;
	int phi_arg, err;
	unsigned int bc_offset;

	insn = bb_first_insn(bb);

	if (insn && insn->type == INSN_PHI) {
		phi_arg = 0;

		for (unsigned long i = 0; i < bb->nr_predecessors; i++) {
			struct basic_block *pred_bb;

			pred_bb = bb->predecessors[i];
			if (bb_is_eh(cu, pred_bb))
				continue;

			bc_offset = bb->start;

			err = insert_copy_insns(cu, pred_bb, &bb, bc_offset, phi_arg);

			if (err)
				return err;

			phi_arg++;
		}
	}

	list_for_each_entry_safe(insn, tmp, &bb->insn_list, insn_list_node) {
		if (insn->type != INSN_PHI)
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

/*
 * insn_add_ons contain the third operand for those
 * instructions that require three operands.
*/
static int create_insn_add_ons_list(struct basic_block *bb)
{
	struct insn *insn;

	INIT_LIST_HEAD(&bb->insn_add_ons_list);

	for_each_insn(insn, &bb->insn_list) {
		struct insn_add_ons *insn_add_ons;

		insn_add_ons = malloc(sizeof(struct insn_add_ons));
		INIT_LIST_HEAD(&insn_add_ons->insn_list_node);
		insn_add_ons->var = NULL;

		list_add_tail(&insn_add_ons->insn_list_node, &bb->insn_add_ons_list);
	}

	return 0;
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

	return 0;

error_dom:
	free_positions_as_predecessor(cu);

error_positions:
	return err;
}

static int lir_to_ssa(struct compilation_unit *cu, struct var_info *gpr)
{
	int err;
	struct basic_block *bb;

	insert_init_insn(cu, gpr);

	err = analyze_def(cu);
	if (err)
		goto error;

	err = insert_phi_insns(cu);
	if (err)
		goto error_def;;

	for_each_basic_block(bb, &cu->bb_list) {
		if (bb_is_eh(cu, bb))
			continue;

		err = create_insn_add_ons_list(bb);
		if (err)
			goto error;
	}

	cu->ssa_nr_vregs = NR_FIXED_REGISTERS;

	err = rename_variables(cu);
	if (err)
		goto error_rename_vars;

	return 0;

error_def:
	free_def_set(cu);
error_rename_vars:
	free_insn_add_ons(cu);

error:
	return err;
}

static int ssa_to_lir(struct compilation_unit *cu)
{
	int err;

	insert_instruction_pass(cu);

	err = ssa_deconstruction(cu);
	if (err)
		return err;

	recompute_insn_positions(cu);

	return 0;
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

/*
 * We add instructions in the entry basic block defining those non-fixed virtual registers
 * that are used before are defined. We do this because, otherwise, the register allocator
 * would not work correctly.
 */
static void repair_use_before_def(struct compilation_unit *cu, struct var_info *gpr)
{
	struct var_info *var;
	struct use_position *this;
	unsigned int min_def, min_use;
	struct live_interval *it;
	struct insn *conv_insn;

	for_each_variable(var, cu->var_infos) {
		it = var->interval;

		if (!interval_has_fixed_reg(it)) {
			min_use = INT_MAX;
			min_def = INT_MAX;

			list_for_each_entry(this, &it->use_positions, use_pos_list) {
				if (insn_vreg_use(this->insn, var)) {
					if (this->insn->lir_pos < min_use)
						min_use = this->insn->lir_pos;
				}
				if (insn_vreg_def(this->insn, var)) {
					if (this->insn->lir_pos < min_def)
						min_def = this->insn->lir_pos;
				}
			}

			if (min_use < min_def)
				insert_def_insn(cu, var, gpr, &conv_insn);
		}
	}

	recompute_insn_positions(cu);
}

int compute_ssa(struct compilation_unit *cu)
{
	int err;
	struct var_info *gpr;

	gpr = get_var(cu, J_INT);

	err = init_ssa(cu);
	if (err)
		goto error_init_ssa;

	err = lir_to_ssa(cu, gpr);
	if (err)
		goto error_lir_ssa;

	if (opt_trace_ssa)
		trace_ssa(cu);

	err = ssa_to_lir(cu);
	if (err)
		goto error;

	replace_var_infos(cu);

	free_ssa(cu);

	create_fixed_var_infos(cu);
	cumulate_fixed_var_infos(cu);

	repair_use_before_def(cu, gpr);

	return 0;

error_init_ssa:
	free_def_set(cu);
	free_positions_as_predecessor(cu);
	free_dom_successors(cu);

error_lir_ssa:
	free_insn_add_ons(cu);

error:
	return err;
}
