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

#include "arch/instruction.h"
#include <stdlib.h>

void free_insn(struct insn *insn)
{
	free(insn);
}

int insn_defs(struct compilation_unit *cu, struct insn *insn, struct var_info **defs)
{
	assert(!"not implemented");
}

int insn_uses(struct insn *insn, struct var_info **uses)
{
	assert(!"not implemented");
}

bool insn_is_branch(struct insn *insn)
{
	assert(!"not implemented");
}

bool insn_is_jmp_branch(struct insn *insn)
{
	assert(!"not implemented");
}

bool insn_is_call(struct insn *insn)
{
	assert(!"not implemented");
}

void insn_sanity_check(void)
{
	assert(!"not implemented");
}

struct insn *insn_new(enum insn_type type)
{
	struct insn *insn;

	insn = calloc(1, sizeof *insn);
	if (insn) {
		memset(insn, 0, sizeof *insn);
		INIT_LIST_HEAD(&insn->insn_list_node);
		INIT_LIST_HEAD(&insn->branch_list_node);
		insn->type = type;
	}
	return insn;
}

struct insn *insn(enum insn_type insn_type)
{
	return insn_new(insn_type);
}

int insert_copy_slot_32_insns(struct stack_slot *stack_slot1, struct stack_slot *stack_slot2, struct list_head *list_head, unsigned long a)
{
	assert(!"not implemented");
}

int insert_copy_slot_64_insns(struct stack_slot *stack_slot1, struct stack_slot *stack_slot2, struct list_head *list_head, unsigned long a)
{
	assert(!"not implemented");
}

struct insn *spill_insn(struct var_info *var, struct stack_slot *slot)
{
	assert(!"not implemented");
}

struct insn *reload_insn(struct stack_slot *slot, struct var_info *var)
{
	assert(!"not implemented");
}

struct insn *jump_insn(struct basic_block *bb)
{
	assert(!"not implemented");
}
