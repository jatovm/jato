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
#include "jit/compilation-unit.h"
#include "jit/compiler.h"
#include "vm/class.h"
#include "vm/method.h"
#include "vm/vm.h"
#include <libharness.h>

struct vm_method method;

static void assert_st_insn(enum insn_type type, struct stack_slot *slot, enum machine_reg reg, struct insn *insn)
{
	assert_int_equals(type, insn->type);
	assert_int_equals(reg, mach_reg(&insn->x.reg));
	assert_ptr_equals(slot, insn->y.slot); 
}

static void assert_ld_insn(enum insn_type type, enum machine_reg reg, struct stack_slot *slot, struct insn *insn)
{
	assert_int_equals(type, insn->type);
	assert_int_equals(reg, mach_reg(&insn->x.reg));
	assert_ptr_equals(slot, insn->y.slot); 
}


void test_spill_insn_is_inserted_at_the_end_of_the_interval_if_necessary(void)
{
        struct compilation_unit *cu;
        struct insn *insn_array[2];
        struct var_info *r1, *r2;
        struct basic_block *bb;
	struct insn *insn;

        cu = compilation_unit_alloc(&method);
        r1 = get_var(cu, J_INT);
        r2 = get_var(cu, J_INT);

        insn_array[0] = arithmetic_insn(INSN_ADD, r1, r1, r1);
        insn_array[1] = arithmetic_insn(INSN_ADD, r2, r2, r2);

        bb = get_basic_block(cu, 0, 2);
        bb_add_insn(bb, insn_array[0]);
        bb_add_insn(bb, insn_array[1]);

	r1->interval->need_spill = true;

	compute_insn_positions(cu);
	analyze_liveness(cu);
	insert_spill_reload_insns(cu);

	/*
	 * First instruction stays the same. 
	 */
	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_ptr_equals(insn_array[0], insn);

	/*
	 * Last instruction stays the same. 
	 */
	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_ptr_equals(insn_array[1], insn);

	/*
	 * A spill instruction is inserted after the interval end.
	 */ 
	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_st_insn(INSN_ST_LOCAL, r1->interval->spill_slot, r1->interval->reg, insn);

	free_compilation_unit(cu);
}

void test_reload_insn_is_inserted_at_the_beginning_of_the_interval_if_necessary(void)
{
        struct compilation_unit *cu;
        struct insn *insn_array[2];
        struct var_info *r1, *r2;
        struct basic_block *bb;
	struct insn *insn;

        cu = compilation_unit_alloc(&method);
        r1 = get_var(cu, J_INT);
        r2 = get_var(cu, J_INT);

        insn_array[0] = arithmetic_insn(INSN_ADD, r1, r1, r1);
        insn_array[1] = arithmetic_insn(INSN_ADD, r2, r2, r2);

        bb = get_basic_block(cu, 0, 2);
        bb_add_insn(bb, insn_array[0]);
        bb_add_insn(bb, insn_array[1]);

	r1->interval->spill_reload_reg.interval = r1->interval;
	r2->interval->spill_reload_reg.interval = r2->interval;

	r2->interval->need_reload = true;
	r2->interval->spill_parent = r1->interval;

	compute_insn_positions(cu);
	analyze_liveness(cu);
	insert_spill_reload_insns(cu);

	/*
	 * A reload instruction is inserted at the beginning.
	 */
	insn = list_first_entry(&bb->insn_list, struct insn, insn_list_node);
	assert_ld_insn(INSN_LD_LOCAL, r2->interval->reg, r1->interval->spill_slot, insn);

	/*
	 * Second instruction stays the same.
	 */
	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_ptr_equals(insn_array[0], insn);

	/*
	 * Last instruction stays the same. 
	 */
	insn = list_next_entry(&insn->insn_list_node, struct insn, insn_list_node);
	assert_ptr_equals(insn_array[1], insn);

	free_compilation_unit(cu);
}

void test_empty_interval_is_never_spilled(void)
{
	struct compilation_unit *cu;
	struct basic_block *bb;
	struct var_info *r1;

	cu = compilation_unit_alloc(&method);
	bb = get_basic_block(cu, 0, 2);

	r1 = get_var(cu, J_INT);
	r1->interval->need_spill = true;

	compute_insn_positions(cu);
	analyze_liveness(cu);
	insert_spill_reload_insns(cu);

	free_compilation_unit(cu);
}
