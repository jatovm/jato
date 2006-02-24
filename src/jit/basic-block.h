#ifndef __JIT_BASIC_BLOCK_H
#define __JIT_BASIC_BLOCK_H

#include <list.h>

struct insn;
struct statement;

struct basic_block {
	unsigned long start;
	unsigned long end;
	struct statement *label_stmt;
	struct statement *stmt;
	struct list_head insn_list;
	struct basic_block *next;
};

struct basic_block *alloc_basic_block(unsigned long, unsigned long);
void free_basic_block(struct basic_block *);
struct basic_block *bb_split(struct basic_block *, unsigned long);
struct basic_block *bb_find(struct basic_block *, unsigned long);
unsigned long nr_bblocks(struct basic_block *);
void bb_insert_insn(struct basic_block *, struct insn *);

#endif
