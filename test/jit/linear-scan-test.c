/*
 * Copyright © 2007  Pekka Enberg
 */

#include <jit/compiler.h>
#include <libharness.h>
#include <vm/vm.h>

struct methodblock method;

void test_allocates_different_registers_for_overlapping_intervals(void)
{
	struct compilation_unit *cu;
	struct var_info *v1, *v2;

	cu = compilation_unit_alloc(&method);

	v1 = get_var(cu);
	v1->interval->range.start = 0;
	v1->interval->range.end   = 2;

	v2 = get_var(cu);
	v2->interval->range.start = 1;
	v2->interval->range.end   = 2;

	allocate_registers(cu);

	assert_int_equals(R0, v1->interval->reg);
	assert_int_equals(R1, v2->interval->reg);

	free_compilation_unit(cu);
}

void test_reuses_registers_for_non_overlapping_intervals(void)
{
	struct compilation_unit *cu;
	struct var_info *v1, *v2;

	cu = compilation_unit_alloc(&method);

	v1 = get_var(cu);
	v1->interval->range.start = 0;
	v1->interval->range.end   = 2;

	v2 = get_var(cu);
	v2->interval->range.start = 2;
	v2->interval->range.end   = 4;

	allocate_registers(cu);

	assert_int_equals(R0, v1->interval->reg);
	assert_int_equals(R0, v2->interval->reg);

	free_compilation_unit(cu);
}

void test_honors_fixed_interval_register_constraint_for_overlapping_intervals(void)
{
	struct compilation_unit *cu;
	struct var_info *v1, *v2;

	cu = compilation_unit_alloc(&method);

	v1 = get_fixed_var(cu, R0);
	v1->interval->range.start = 0;
	v1->interval->range.end   = 2;

	v2 = get_var(cu);
	v2->interval->range.start = 0;
	v2->interval->range.end   = 2;

	allocate_registers(cu);

	assert_int_equals(R0, v1->interval->reg);
	assert_int_equals(R1, v2->interval->reg);

	free_compilation_unit(cu);
}
