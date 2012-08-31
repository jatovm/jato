/*
 * Copyright (c) 2006 Tomasz Grabiec
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
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
