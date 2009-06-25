/*
 * Copyright (c) 2006-2008  Pekka Enberg
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
#include <jit/basic-block.h>
#include <jit/compilation-unit.h>
#include <jit/stack-slot.h>
#include <jit/vars.h>
#include <vm/buffer.h>
#include <vm/method.h>
#include <vm/vm.h>

#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct compilation_unit *compilation_unit_alloc(struct vm_method *method)
{
	struct compilation_unit *cu = malloc(sizeof *cu);
	if (cu) {
		memset(cu, 0, sizeof *cu);

		INIT_LIST_HEAD(&cu->bb_list);
		cu->method = method;
		cu->is_compiled = false;

		cu->exit_bb = alloc_basic_block(cu, 0, 0);
		if (!cu->exit_bb)
			goto out_of_memory;

		cu->unwind_bb = alloc_basic_block(cu, 0, 0);
		if (!cu->unwind_bb)
			goto out_of_memory;

		pthread_mutex_init(&cu->mutex, NULL);

		cu->stack_frame = alloc_stack_frame(
			method->args_count,
			method->code_attribute.max_locals);
		if (!cu->stack_frame)
			goto out_of_memory;

		cu->exception_spill_slot = get_spill_slot_32(cu->stack_frame);
		if (!cu->exception_spill_slot)
			goto out_of_memory;
	}

	return cu;

out_of_memory:
	free_compilation_unit(cu);
	return NULL;
}

static void free_var_info(struct var_info *var)
{
	free_interval(var->interval);
	free(var);
}

static void free_var_infos(struct var_info *var_infos)
{
	struct var_info *this, *next;

	for (this = var_infos; this != NULL; this = next) {
		next = this->next;
		free_var_info(this);
	}
}

void free_compilation_unit(struct compilation_unit *cu)
{
	struct basic_block *bb, *tmp_bb;

	list_for_each_entry_safe(bb, tmp_bb, &cu->bb_list, bb_list_node)
		free_basic_block(bb);

	pthread_mutex_destroy(&cu->mutex);
	free_basic_block(cu->exit_bb);
	free_basic_block(cu->unwind_bb);
	free_buffer(cu->objcode);
	free_var_infos(cu->var_infos);
	free_stack_frame(cu->stack_frame);
	free(cu);
}

struct var_info *get_var(struct compilation_unit *cu)
{
	struct var_info *ret;

	ret = malloc(sizeof *ret);
	if (!ret)
		goto out;

	ret->vreg = cu->nr_vregs++;
	ret->next = cu->var_infos;
	ret->interval = alloc_interval(ret);

	cu->var_infos = ret;
  out:
	return ret;
}

struct var_info *get_fixed_var(struct compilation_unit *cu, enum machine_reg reg)
{
	struct var_info *ret;

	ret = get_var(cu);
	if (ret) {
		ret->interval->reg = reg;
		ret->interval->fixed_reg = true;
	}

	return ret;
}

/**
 * 	bb_find - Find basic block containing @offset.
 * 	@bb_list: First basic block in list.
 * 	@offset: Offset to find.
 * 
 * 	Find the basic block that contains the given offset and returns a
 * 	pointer to it.
 */
struct basic_block *find_bb(struct compilation_unit *cu, unsigned long offset)
{
	struct basic_block *bb;

	for_each_basic_block(bb, &cu->bb_list) {
		if (offset >= bb->start && offset < bb->end)
			return bb;
	}
	return NULL;
}

unsigned long nr_bblocks(struct compilation_unit *cu)
{
	struct basic_block *bb;
	unsigned long nr = 0;

	for_each_basic_block(bb, &cu->bb_list) {
		nr++;
	}

	return nr;
}

void compute_insn_positions(struct compilation_unit *cu)
{
	struct basic_block *bb;
	unsigned long pos = 0;

	for_each_basic_block(bb, &cu->bb_list) {
		struct insn *insn;

		for_each_insn(insn, &bb->insn_list) {
			insn->lir_pos = pos++;
		}
	}
}

static int bb_list_compare(const struct list_head **e1, const struct list_head **e2)
{
	struct basic_block *bb1, *bb2;

	bb1 = list_entry(*e1, struct basic_block, bb_list_node);
	bb2 = list_entry(*e2, struct basic_block, bb_list_node);

	return bb1->start - bb2->start;
}

int sort_basic_blocks(struct compilation_unit *cu)
{
	return list_sort(&cu->bb_list, bb_list_compare);
}
