#ifndef __ARGS_H
#define __ARGS_H

#include "jit/expression.h"

#include "vm/stack.h"

struct expression *insert_arg(struct expression *root, struct expression *expr);
struct expression *convert_args(struct stack *mimic_stack, unsigned long nr_args);
const char *parse_method_args(const char *type_str, enum vm_type *vmtype);

#endif
