#ifndef __JIT_BASIC_BLOCK_H
#define __JIT_BASIC_BLOCK_H

#include <vm/list.h>

struct compilation_unit;
struct insn;
struct statement;

struct basic_block {
	struct compilation_unit *b_parent;
	unsigned long start;
	unsigned long end;
	struct list_head stmt_list;
	struct list_head insn_list;
	struct list_head branch_list;		/* List of incoming branches */
	struct list_head bb_list_node;
};

static inline struct basic_block *bb_entry(struct list_head *head)
{
	return list_entry(head, struct basic_block, bb_list_node);
}

struct basic_block *alloc_basic_block(struct compilation_unit *, unsigned long, unsigned long);
void free_basic_block(struct basic_block *);
struct basic_block *bb_split(struct basic_block *, unsigned long);
void bb_add_stmt(struct basic_block *, struct statement *);
void bb_add_insn(struct basic_block *, struct insn *);

#endif
