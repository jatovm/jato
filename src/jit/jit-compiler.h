#ifndef __JIT_COMPILER_H
#define __JIT_COMPILER_H

#include <basic-block.h>
#include <stack.h>

#define MAX_BASIC_BLOCKS 1

struct compilation_unit {
	struct classblock *cb;
	unsigned char *code;
	struct basic_block basic_blocks[MAX_BASIC_BLOCKS];
	struct stack *expr_stack;
};

int convert_to_ir(struct compilation_unit *);

#endif
