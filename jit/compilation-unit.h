#ifndef __JIT_COMPILATION_UNIT_H
#define __JIT_COMPILATION_UNIT_H

#include <list.h>
#include <basic-block.h>
#include <stack.h>

struct compilation_unit {
	struct classblock *cb;
	unsigned char *code;
	unsigned long code_len;
	struct list_head bb_list;
	struct stack *expr_stack;
	void *objcode;
};

struct compilation_unit *alloc_compilation_unit(unsigned char *,
						unsigned long);
void free_compilation_unit(struct compilation_unit *);
struct basic_block *find_bb(struct compilation_unit *, unsigned long);
unsigned long nr_bblocks(struct compilation_unit *);

#endif
