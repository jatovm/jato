#ifndef __ARGS_H
#define __ARGS_H

#include "jit/expression.h"

#include "vm/method.h"
#include "lib/stack.h"

#include <assert.h>

struct expression **pop_args(struct stack *mimic_stack, unsigned long nr_args);
struct expression *convert_args(struct expression **args_array, unsigned long nr_args, struct vm_method *method);
struct expression *convert_native_args(struct stack *mimic_stack, unsigned long start_arg, unsigned long nr_args);

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
extern int args_map_init(struct vm_method *method);

static inline int get_stack_args_count(struct vm_method *method)
{
	long size;

	size = method->args_count;

	if (vm_method_is_jni(method))
		size++;

	return size - method->reg_args_count;
}
#endif /* CONFIG_ARGS_MAP */

#endif
