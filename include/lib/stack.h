#ifndef LIB_STACK_H
#define LIB_STACK_H

#include <assert.h>
#include <errno.h>
#include <memory.h>
#include <stdbool.h>
#include <stdlib.h>

struct stack {
	unsigned long	nr_elements;
	void		**elements;
};

struct stack *alloc_stack(void);
void free_stack(struct stack *);

static inline void *stack_pop(struct stack *stack)
{
	assert(stack->nr_elements > 0);

	return stack->elements[--stack->nr_elements];
}

static inline void *stack_peek(struct stack *stack)
{
	assert(stack->nr_elements > 0);

	return stack->elements[stack->nr_elements - 1];
}

static inline void stack_push(struct stack *stack, void *entry)
{
	void *p;

	p = realloc(stack->elements, sizeof(void *) * (stack->nr_elements + 1));
	assert(p);

	stack->elements = p;

	stack->elements[stack->nr_elements++] = entry;
}

static inline void stack_clear(struct stack *stack)
{
	stack->nr_elements = 0;
}

static inline bool stack_is_empty(struct stack *stack)
{
	return !stack->nr_elements;
}

static inline unsigned long stack_size(struct stack *stack)
{
	return stack->nr_elements;
}

void stack_copy(struct stack *src, struct stack *dst);

#endif
