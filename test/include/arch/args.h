#ifndef MMIX_ARGS_H
#define MMIX_ARGS_H

#include "arch/registers.h"

#include "jit/expression.h"

#include "vm/method.h"

static inline void args_finish(unsigned long *state)
{
}

static inline int args_init(unsigned long *state,
			    struct vm_method *method,
			    unsigned long nr_args)
{
	return 0;
}

static inline int args_set(unsigned long *state,
			   struct vm_method *method,
			   struct expression *expr)
{
	return 0;
}

#endif /* MMIX_ARGS_H */
