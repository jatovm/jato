#ifndef __JIT_BASIC_BLOCK_H
#define __JIT_BASIC_BLOCK_H

#include <vm/list.h>
#include <stdbool.h>

struct compilation_unit;
struct insn;
struct statement;

struct basic_block {
	struct compilation_unit *b_parent;
	unsigned long start;
	unsigned long end;
	bool is_emitted;
	struct list_head stmt_list;
	struct list_head insn_list;
	/* List of forward branch instructions to this basic block that need
	   back-patching.  */
	struct list_head backpatch_insns;
	struct list_head bb_list_node;
};

static inline struct basic_block *bb_entry(struct list_head *head)
{
	return list_entry(head, struct basic_block, bb_list_node);
}

struct basic_block *alloc_basic_block(struct compilation_unit *, unsigned long, unsigned long);
struct basic_block *get_basic_block(struct compilation_unit *, unsigned long, unsigned long);
void free_basic_block(struct basic_block *);
struct basic_block *bb_split(struct basic_block *, unsigned long);
void bb_add_stmt(struct basic_block *, struct statement *);
void bb_add_insn(struct basic_block *, struct insn *);

#define for_each_basic_block(bb, bb_list) list_for_each_entry(bb, bb_list, bb_list_node)

#endif
