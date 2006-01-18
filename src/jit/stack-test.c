/*
 * Copyright (C) 2005  Pekka Enberg
 */

#include <stack.h>

#include <CuTest.h>
#include <stdlib.h>

void test_stack_retains_elements(CuTest *ct)
{
	struct stack stack = STACK_INIT;
	stack_push(&stack, 0);
	stack_push(&stack, 1);
	CuAssertIntEquals(ct, 1, stack_pop(&stack));
	CuAssertIntEquals(ct, 0, stack_pop(&stack));
}
