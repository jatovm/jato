/*
 * Liveness analysis
 * Copyright (C) 2007  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <jit/compilation-unit.h>
#include <jit/vars.h>
#include <vm/bitset.h>

#include <errno.h>
#include <stdlib.h>

static void __update_interval_insn(struct live_interval *interval,
				   struct basic_block *bb)
{
	struct insn *insn;

	for_each_insn(insn, &bb->insn_list) {
		unsigned long offset;

		if (!in_range(&interval->range, insn->lir_pos))
			continue;

		offset = insn->lir_pos - interval->range.start;
		interval->insn_array[offset] = insn;
	}
}

static int update_interval_insns(struct compilation_unit *cu)
{
	struct var_info *var;
	int err = 0;

	for_each_variable(var, cu->var_infos) {
		struct live_interval *interval = var->interval;
		struct basic_block *bb;
		unsigned long nr;

		nr = range_len(&interval->range);
		interval->insn_array = calloc(nr, sizeof(struct insn *));
		if (!interval->insn_array) {
			err = -ENOMEM;
			break;
		}
		for_each_basic_block(bb, &cu->bb_list) {
			__update_interval_insn(var->interval, bb);
		}
	}
	return err;
}

static void __update_live_range(struct live_range *range, unsigned long pos)
{
	if (range->start > pos)
		range->start = pos;

	if (range->end < pos)
		range->end = pos;
}

static void update_live_ranges(struct compilation_unit *cu)
{
	struct basic_block *this;

	for_each_basic_block(this, &cu->bb_list) {
		struct var_info *var;

		for_each_variable(var, cu->var_infos) {
			if (test_bit(this->live_in_set->bits, var->vreg))
				__update_live_range(&var->interval->range, this->start_insn);

			if (test_bit(this->live_out_set->bits, var->vreg))
				__update_live_range(&var->interval->range, this->end_insn);
		}
	}
}

static int analyze_live_sets(struct compilation_unit *cu)
{
	struct bitset *old_live_out_set = NULL;
	struct bitset *old_live_in_set = NULL;
	struct basic_block *this;
	bool changed;
	int err = 0;

	old_live_in_set = alloc_bitset(cu->nr_vregs);
	if (!old_live_in_set) {
		err = -ENOMEM;
		goto out;
	}

	old_live_out_set = alloc_bitset(cu->nr_vregs);
	if (!old_live_out_set) {
		err = -ENOMEM;
		goto out;
	}

	/*
	 * This is your standard text-book iterative liveness analysis
	 * algorithm for computing live-in and live-out sets for each
	 * basic block.
	 */
	do {
		changed = false;

		for_each_basic_block_reverse(this, &cu->bb_list) {
			struct basic_block *succ;
			unsigned long i;

			bitset_copy_to(this->live_in_set, old_live_in_set);
			bitset_copy_to(this->live_out_set, old_live_out_set);

			bitset_copy_to(this->live_out_set, this->live_in_set);
			bitset_sub(this->def_set, this->live_in_set);
			bitset_union_to(this->use_set, this->live_in_set);

			bitset_clear_all(this->live_out_set);
			for (i = 0; i < this->nr_successors; i++) {
				succ = this->successors[i];
				bitset_union_to(succ->live_in_set, this->live_out_set);
			}

			/*
			 * If live-in or live-out set changed, we must
			 * iterate at least once more.
			 */
			if (!bitset_equal(this->live_in_set, old_live_in_set) ||
			    !bitset_equal(this->live_out_set, old_live_out_set))
				changed = true;
		}
	} while (changed);
  out:
	free(old_live_out_set);
	free(old_live_in_set);
	return err;
}

static void __analyze_use_def(struct basic_block *bb, struct insn *insn)
{
	struct var_info *var;

	for_each_variable(var, bb->b_parent->var_infos) {
		if (insn_defs(insn, var->vreg))
			set_bit(bb->def_set->bits, var->vreg);

		if (insn_uses(insn, var->vreg))
			set_bit(bb->use_set->bits, var->vreg);
	}

	/*
	 * It's in the use set if and only if it has not been defined
	 * by insn basic block.
	 */
	bitset_sub(bb->def_set, bb->use_set);
}

static void analyze_use_def(struct compilation_unit *cu)
{
	struct basic_block *bb;

	for_each_basic_block(bb, &cu->bb_list) {
		struct insn *insn;

		for_each_insn(insn, &bb->insn_list) {
			struct var_info *var;

			__analyze_use_def(bb, insn);

			for_each_variable(var, bb->b_parent->var_infos) {
				if (test_bit(bb->def_set->bits, var->vreg) ||
				    test_bit(bb->use_set->bits, var->vreg))
					__update_live_range(&var->interval->range, insn->lir_pos);
			}
		}
	}
}

static int __init_sets(struct basic_block *bb, unsigned long nr_vregs)
{
	bb->use_set = alloc_bitset(nr_vregs);
	if (!bb->use_set)
		return -ENOMEM;

	bb->def_set = alloc_bitset(nr_vregs);
	if (!bb->def_set)
		return -ENOMEM;

	bb->live_in_set = alloc_bitset(nr_vregs);
	if (!bb->live_in_set)
		return -ENOMEM;

	bb->live_out_set = alloc_bitset(nr_vregs);
	if (!bb->live_out_set)
		return -ENOMEM;

	return 0;
}

static int init_sets(struct compilation_unit *cu)
{
	struct basic_block *this;
	int err = 0;

	for_each_basic_block(this, &cu->bb_list) {
		err = __init_sets(this, cu->nr_vregs);
		if (err)
			break;
	}
	return err;
}

static void compute_boundaries(struct compilation_unit *cu)
{
	unsigned long insn_idx = 0;
	struct basic_block *bb;
	struct insn *insn;

	for_each_basic_block(bb, &cu->bb_list) {
		bb->start_insn = insn_idx;
		for_each_insn(insn, &bb->insn_list) {
			insn_idx++;
		}
		bb->end_insn = insn_idx;
	}
}

int analyze_liveness(struct compilation_unit *cu)
{
	int err = 0;

	compute_boundaries(cu);

	err = init_sets(cu);
	if (err)
		goto out;

	analyze_use_def(cu);

	err = analyze_live_sets(cu);
	if (err)
		goto out;

	update_live_ranges(cu);

	err = update_interval_insns(cu);
  out:
	return err;
}
