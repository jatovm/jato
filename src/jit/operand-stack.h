#ifndef __OPERAND_STACK_H
#define __OPERAND_STACK_H

#include <assert.h>
#include <stdbool.h>

enum { MAX_ELEMENTS = 255 };

struct operand_stack {
	unsigned long nr_elements;
	unsigned long elements[MAX_ELEMENTS];
};

#define OPERAND_STACK_INIT { .nr_elements = 0 }

static inline unsigned long stack_pop(struct operand_stack * stack)
{
	assert(stack->nr_elements);
	return stack->elements[--stack->nr_elements];
}

static inline void stack_push(struct operand_stack * stack, unsigned long entry)
{
	assert(stack->nr_elements < MAX_ELEMENTS);
	stack->elements[stack->nr_elements++] = entry;
}

static inline bool stack_is_empty(struct operand_stack * stack)
{
	return !stack->nr_elements;
}

#endif
