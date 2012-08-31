#include "lib/stack.h"

#include <libharness.h>
#include <stdlib.h>

void test_stack_retains_elements(void)
{
	struct stack *stack = alloc_stack();

	stack_push(stack, (void *)0);
	stack_push(stack, (void *)1);
	assert_ptr_equals((void *)1, stack_pop(stack));
	assert_ptr_equals((void *)0, stack_pop(stack));

	free_stack(stack);
}

void test_stack_reverse(void)
{
	struct stack *stack = alloc_stack();

	stack_push(stack, (void *)0);
	stack_push(stack, (void *)1);
	stack_push(stack, (void *)2);

	stack_reverse(stack);

	assert_ptr_equals((void *)0, stack_pop(stack));
	assert_ptr_equals((void *)1, stack_pop(stack));
	assert_ptr_equals((void *)2, stack_pop(stack));

	free_stack(stack);
}
