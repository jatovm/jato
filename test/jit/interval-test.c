/*
 * Copyright (c) 2008  Pekka Enberg
 */
#include <arch/instruction.h>
#include <jit/vars.h>
#include <libharness.h>
#include <stdlib.h>

void test_split_interval_at(void)
{
	struct live_interval *new_interval;
	struct insn insns[2];
	struct var_info var;
	int i;

	var.interval = alloc_interval(&var);
	var.interval->insn_array = calloc(ARRAY_SIZE(insns), sizeof(struct insn *));

	for (i = 0; i < ARRAY_SIZE(insns); i++) {
		insns[i].lir_pos = i;
		assoc_var_to_operand(&var, &insns[i], x.reg);
		var.interval->insn_array[i] = &insns[i];
	}
	var.interval->range.start = 0;
	var.interval->range.end = ARRAY_SIZE(insns);

	assert_ptr_equals(var.interval, insns[0].x.reg.interval);
	assert_ptr_equals(var.interval, insns[1].x.reg.interval);

	new_interval = split_interval_at(var.interval, 1);

	assert_ptr_equals(var.interval, insns[0].x.reg.interval);
	assert_int_equals(0, var.interval->range.start);
	assert_int_equals(1, var.interval->range.end);
	assert_ptr_equals(&insns[0], var.interval->insn_array[0]);

	assert_ptr_equals(new_interval, insns[1].x.reg.interval);
	assert_int_equals(1, new_interval->range.start);
	assert_int_equals(2, new_interval->range.end);
	assert_ptr_equals(&insns[1], new_interval->insn_array[0]);

	free_interval(new_interval);
}
