#ifndef __ARGS_H
#define __ARGS_H

#include <jit/expression.h>

#include <vm/stack.h>

struct expression *insert_arg(struct expression *root,struct expression *expr);
struct expression *convert_args(struct stack *expr_stack,unsigned long nr_args);

#endif
