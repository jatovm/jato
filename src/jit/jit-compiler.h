#ifndef __JIT_COMPILER_H
#define __JIT_COMPILER_H

#include <stack.h>

#define BASIC_BLOCK_INIT(_code, _len) \
	{ .code = _code, .len = _len }

struct basic_block {
	unsigned long len;
	unsigned char *code;
	struct statement *stmt;
};

struct compilation_unit {
	struct classblock *cb;
	struct basic_block basic_block;
	struct stack *expr_stack;
};

int convert_to_ir(struct compilation_unit *);

#endif
