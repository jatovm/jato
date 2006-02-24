#ifndef __JIT_COMPILATION_UNIT_H
#define __JIT_COMPILATION_UNIT_H

#include <basic-block.h>
#include <stack.h>

struct compilation_unit {
	struct classblock *cb;
	unsigned char *code;
	unsigned long code_len;
	struct basic_block *entry_bb;
	struct stack *expr_stack;
};

struct compilation_unit *alloc_compilation_unit(unsigned char *,
						unsigned long,
						struct basic_block *,
						struct stack *);
void free_compilation_unit(struct compilation_unit *);
struct basic_block *bb_find(struct basic_block *, unsigned long);

#endif
