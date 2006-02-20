#ifndef __JIT_COMPILER_H
#define __JIT_COMPILER_H

#include <basic-block.h>
#include <stack.h>

struct compilation_unit {
	struct classblock *cb;
	unsigned char *code;
	unsigned long code_len;
	struct basic_block basic_blocks[1];
	struct basic_block *entry_bb;
	struct stack *expr_stack;
};

extern unsigned char bytecode_sizes[];

void build_cfg(struct compilation_unit *);
int convert_to_ir(struct compilation_unit *);

#endif
