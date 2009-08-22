#ifndef __ARGS_H
#define __ARGS_H

#include "arch/args.h"

#include "jit/expression.h"

#include "vm/method.h"
#include "lib/stack.h"

struct expression *insert_arg(struct expression *root,
			      struct expression *expr,
			      struct vm_method *method,
			      int index);
struct expression *convert_args(struct stack *mimic_stack,
				unsigned long nr_args,
				struct vm_method *method);
struct expression *convert_native_args(struct stack *mimic_stack,
				       unsigned long nr_args);

#ifndef CONFIG_ARGS_MAP
static inline int args_map_init(struct vm_method *method)
{
	return 0;
}

static inline int get_stack_args_count(struct vm_method *method)
{
	return method->args_count;
}
#else
static inline int get_stack_args_count(struct vm_method *method)
{
	return method->args_count - method->reg_args_count;
}
#endif /* CONFIG_ARGS_MAP */

#endif
