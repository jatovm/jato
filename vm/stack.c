/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <vm/stack.h>

#include <stdlib.h>
#include <string.h>

struct stack *alloc_stack(void)
{
	struct stack *stack = malloc(sizeof(*stack));
	if (stack)
		memset(stack, 0, sizeof(*stack));
	return stack;
}

void free_stack(struct stack *stack)
{
	free(stack);
}
