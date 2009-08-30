/*
 * Copyright © 2007  Pekka Enberg
 */

#include "jit/compiler.h"
#include "arch/instruction.h"
#include <libharness.h>
#include "vm/class.h"
#include "vm/method.h"
#include "vm/vm.h"

struct vm_method method;

void test_allocates_different_registers_for_overlapping_intervals(void)
{
	struct compilation_unit *cu;
	struct var_info *v1, *v2;

	cu = compilation_unit_alloc(&method);

	v1 = get_var(cu, J_INT);
	interval_add_range(v1->interval, 0, 2);

	v2 = get_var(cu, J_INT);
	interval_add_range(v2->interval, 1, 2);

	allocate_registers(cu);

	assert(v1->interval->reg != v2->interval->reg);

	free_compilation_unit(cu);
}

void test_reuses_registers_for_non_overlapping_intervals(void)
{
	struct compilation_unit *cu;
	struct var_info *v1, *v2;

	cu = compilation_unit_alloc(&method);

	v1 = get_var(cu, J_INT);
	interval_add_range(v1->interval, 0, 2);

	v2 = get_var(cu, J_INT);
	interval_add_range(v2->interval, 2, 4);

	allocate_registers(cu);

	assert_int_equals(v1->interval->reg, v2->interval->reg);

	free_compilation_unit(cu);
}

void test_honors_fixed_interval_register_constraint_for_overlapping_intervals(void)
{
	struct compilation_unit *cu;
	struct var_info *v1, *v2;

	cu = compilation_unit_alloc(&method);

	v1 = get_fixed_var(cu, R0);
	interval_add_range(v1->interval, 0, 2);

	v2 = get_var(cu, J_INT);
	interval_add_range(v2->interval, 0, 2);

	allocate_registers(cu);

	assert(v1->interval->reg != v2->interval->reg);

	free_compilation_unit(cu);
}
