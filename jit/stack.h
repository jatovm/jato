#ifndef __STACK_H
#define __STACK_H

#include <assert.h>
#include <stdbool.h>

enum { MAX_ELEMENTS = 255 };

struct stack {
	unsigned long nr_elements;
	void *elements[MAX_ELEMENTS];
};

#define STACK_INIT { .nr_elements = 0 }

static inline void *stack_pop(struct stack * stack)
{
	assert(stack->nr_elements);
	return stack->elements[--stack->nr_elements];
}

static inline void stack_push(struct stack * stack, void *entry)
{
	assert(stack->nr_elements < MAX_ELEMENTS);
	stack->elements[stack->nr_elements++] = entry;
}

static inline bool stack_is_empty(struct stack * stack)
{
	return !stack->nr_elements;
}

#endif
