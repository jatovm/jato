#ifndef __OPERAND_STACK_H
#define __OPERAND_STACK_H

#include <assert.h>
#include <stdbool.h>

struct operand_stack {
	unsigned long nr_elements;
	unsigned long entry;
};

#define OPERAND_STACK_INIT { .nr_elements = 0, .entry = 0 }

static inline unsigned long stack_pop(struct operand_stack * stack)
{
	assert(stack->nr_elements);
	stack->nr_elements--;
	return stack->entry;
}

static inline void stack_push(struct operand_stack * stack, unsigned long entry)
{
	stack->nr_elements++;
	stack->entry = entry;
}

static inline bool stack_is_empty(struct operand_stack * stack)
{
	return !stack->nr_elements;
}

#endif
