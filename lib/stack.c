/*
 * Copyright (c) 2006 Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "lib/stack.h"

#include "vm/die.h"

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
	free(stack->elements);
	free(stack);
}

void stack_copy(struct stack *src, struct stack *dst)
{
	void **new_elements;
	unsigned long size;

	size = src->nr_elements * sizeof(void*);
	new_elements = realloc(dst->elements, size);
	if (!new_elements)
		error("out of memory");

	dst->elements = new_elements;
	dst->nr_elements = src->nr_elements;
	memcpy(new_elements, src->elements, size);
}

void stack_reverse(struct stack *stack)
{
	unsigned int i;

	for (i = 0; i < stack->nr_elements / 2; i++) {
		unsigned int end = stack->nr_elements - i - 1;
		void *tmp;

		tmp			= stack->elements[i];

		stack->elements[i]	= stack->elements[end];

		stack->elements[end]	= tmp;
	}
}
