#ifndef __JIT_BASIC_BLOCK_H
#define __JIT_BASIC_BLOCK_H

#include <list.h>

struct insn;
struct statement;

struct basic_block {
	unsigned long start;
	unsigned long end;
	struct statement *label_stmt;
	struct list_head stmt_list;
	struct list_head insn_list;
	struct list_head bb_list_node;
};

static inline struct basic_block *bb_entry(struct list_head *head)
{
	return list_entry(head, struct basic_block, bb_list_node);
}

struct basic_block *alloc_basic_block(unsigned long, unsigned long);
void free_basic_block(struct basic_block *);
struct basic_block *bb_split(struct basic_block *, unsigned long);
void bb_insert_stmt(struct basic_block *, struct statement *);
void bb_insert_insn(struct basic_block *, struct insn *);

#endif
