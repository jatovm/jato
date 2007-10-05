/*
 * Copyright © 2007  Pekka Enberg
 */

#include <jit/jit-compiler.h>
#include <libharness.h>

void test_allocates_different_registers_for_overlapping_intervals(void)
{
	struct compilation_unit *cu;
	struct var_info *v1, *v2;

	cu = alloc_compilation_unit(NULL);

	v1 = get_var(cu);
	v1->interval.range.start = 0;
	v1->interval.range.end   = 2;

	v2 = get_var(cu);
	v2->interval.range.start = 1;
	v2->interval.range.end   = 2;

	allocate_registers(cu);

	assert_int_equals(R0, v1->reg);
	assert_int_equals(R1, v2->reg);

	free_compilation_unit(cu);
}

void test_reuses_registers_for_non_overlapping_intervals(void)
{
	struct compilation_unit *cu;
	struct var_info *v1, *v2;

	cu = alloc_compilation_unit(NULL);

	v1 = get_var(cu);
	v1->interval.range.start = 0;
	v1->interval.range.end   = 2;

	v2 = get_var(cu);
	v2->interval.range.start = 2;
	v2->interval.range.end   = 4;

	allocate_registers(cu);

	assert_int_equals(R0, v1->reg);
	assert_int_equals(R0, v2->reg);

	free_compilation_unit(cu);
}

void test_honors_fixed_interval_register_constraint_for_overlapping_intervals(void)
{
	struct compilation_unit *cu;
	struct var_info *v1, *v2;

	cu = alloc_compilation_unit(NULL);

	v1 = get_var(cu);
	v1->interval.range.start = 0;
	v1->interval.range.end   = 2;
	v1->reg = R0;

	v2 = get_var(cu);
	v2->interval.range.start = 0;
	v2->interval.range.end   = 2;

	allocate_registers(cu);

	assert_int_equals(R0, v1->reg);
	assert_int_equals(R1, v2->reg);

	free_compilation_unit(cu);
}
