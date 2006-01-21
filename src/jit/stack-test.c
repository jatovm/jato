/*
 * Copyright (C) 2005  Pekka Enberg
 */

#include <stack.h>

#include <CuTest.h>
#include <stdlib.h>

void test_stack_retains_elements(CuTest *ct)
{
	struct stack stack = STACK_INIT;
	stack_push(&stack, (void *)0);
	stack_push(&stack, (void *)1);
	CuAssertPtrEquals(ct, (void *)1, stack_pop(&stack));
	CuAssertPtrEquals(ct, (void *)0, stack_pop(&stack));
}
