/*
 * Copyright (C) 2005  Pekka Enberg
 */

#include <stack.h>

#include <libharness.h>
#include <stdlib.h>

void test_stack_retains_elements(void)
{
	struct stack stack = STACK_INIT;
	stack_push(&stack, (void *)0);
	stack_push(&stack, (void *)1);
	assert_ptr_equals((void *)1, stack_pop(&stack));
	assert_ptr_equals((void *)0, stack_pop(&stack));
}
