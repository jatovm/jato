#ifndef __X86_ARGS_H
#define __X86_ARGS_H

#include "arch/registers.h"

#include "jit/expression.h"

#include "vm/method.h"

static inline void args_finish(unsigned long *state)
{
}

#ifdef CONFIG_X86_64

extern int args_init(unsigned long *state,
		     struct vm_method *method,
		     unsigned long nr_args);
extern int args_set(unsigned long *state,
		    struct vm_method *method,
		    struct expression *expr);
extern enum machine_reg args_select_reg(struct expression *expr);
extern enum machine_reg args_local_to_reg(struct vm_method *method,
					  int local_idx);

static inline int args_stack_index(struct vm_method *method, int local_idx)
{
	return local_idx - method->reg_args_count;
}

#else /* CONFIG_X86_64 */

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

#endif /* CONFIG_X86_64 */

#endif /* __X86_ARGS_H */
