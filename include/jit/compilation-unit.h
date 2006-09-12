#ifndef __JIT_COMPILATION_UNIT_H
#define __JIT_COMPILATION_UNIT_H

#include <vm/list.h>
#include <jit/basic-block.h>
#include <vm/stack.h>
#include <stdbool.h>
#include <pthread.h>

struct buffer;

struct compilation_unit {
	struct methodblock *method;
	struct list_head bb_list;
	struct basic_block *exit_bb;
	struct stack *expr_stack;
	struct buffer *objcode_buf;
	bool is_compiled;
	pthread_mutex_t mutex;
};

struct compilation_unit *alloc_compilation_unit(struct methodblock *);
void free_compilation_unit(struct compilation_unit *);
struct basic_block *find_bb(struct compilation_unit *, unsigned long);
unsigned long nr_bblocks(struct compilation_unit *);

#endif
