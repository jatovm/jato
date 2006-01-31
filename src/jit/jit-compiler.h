#ifndef __JIT_COMPILER_H
#define __JIT_COMPILER_H

#include <stack.h>

struct compilation_unit {
	struct classblock *cb;
	unsigned char *code;
	unsigned long len;
	struct stack *expr_stack;
	struct statement *stmt;
};

int convert_to_ir(struct compilation_unit *);

#endif
