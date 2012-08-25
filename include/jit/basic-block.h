#ifndef __JIT_BASIC_BLOCK_H
#define __JIT_BASIC_BLOCK_H

#include "lib/list.h"
#include <stdbool.h>

struct compilation_unit;
struct insn;
struct statement;

struct resolution_block {
	unsigned long			addr;
	unsigned long			mach_offset;
	struct list_head		insns;
};

struct basic_block {
	struct compilation_unit *b_parent;
	unsigned long start;
	unsigned long end;
	bool is_converted;
	bool is_emitted;
	struct list_head stmt_list;
	struct list_head insn_list;
	struct list_head bb_list_node;
	bool has_branch;
	unsigned long br_target_off;	/* Branch target offset in bytecode insns. */
	unsigned long nr_successors;
	struct basic_block **successors;
	unsigned long nr_predecessors;
	struct basic_block **predecessors;
	unsigned long nr_mimic_stack_expr;
	struct expression **mimic_stack_expr;
	unsigned long mach_offset;
	int dfn;

	/* Backend specific private data. */
	void *priv;

	/* Contains data flow resultion blocks for each outgoing CFG edge. */
	struct resolution_block *resolution_blocks;

	/* The mimic stack is used to simulate JVM operand stack at
	   compile-time.  See Section 2.2 ("Lazy code selection") of the paper
	   "Fast, Effective Code Generation in a Just-In-Time Java Compiler" by
	   Adl-Tabatabai et al (1998) for more in-depth explanation.  */
	struct stack *mimic_stack;

	/* Holds the size of mimic stack at basic block entry. If
	   mimic stack has not yet been resolved it's set to -1. */
	long entry_mimic_stack_size;

	/* Is this basic block an exception handler? */
	bool is_eh;

	/*
	 * These are computed by SSA.
	 */
	struct {
		/* Dominance frontier set of the basic block. */
		struct bitset *dom_frontier;

		/* Number of predecessors excluding exception handler blocks. */
		unsigned long nr_predecessors_no_eh;

		/*
		 * Position of the current basic block in the predecessors
		 * list of every successor of the basic block.
		 */
		unsigned long *positions_as_predecessor;

		/* Succesors of the basic block along the dominator tree. */
		struct basic_block **dom_successors;
		unsigned long nr_dom_successors;

		/* Dominator basic blocks of the basic block */
		struct bitset *dominators;

		/*
		 * Basic blocks situated in the loop for which
		 * this basic block represents the header
		 */
		struct bitset *natural_loop;

		/*
		 * Variable that keeps the level of nesting of this basic block
		 * within natural loops
		 */
		int nesting;

		/*
		 * Variable that indicates whether or not this basic block is
		 * the starting basic block.
		 */
		int loop_start;

		/*
		 * Flag used for exception handler basic blocks, to mark
		 * the fact that they have been renamed or not
		 */
		bool is_renamed;
	};

	/*
	 * These are computed by liveness analysis.
	 */
	struct {
		unsigned long start_insn, end_insn;

		/* Set of variables defined by this basic block.  */
		struct bitset *def_set;

		/* Set of variables used by this basic block.  */
		struct bitset *use_set;

		/* Set of variables that are live when entering this basic block.  */
		struct bitset *live_in_set;

		/* Set of variables that are live when exiting this basic block.  */
		struct bitset *live_out_set;
	};
};

static inline struct basic_block *bb_entry(struct list_head *head)
{
	return list_entry(head, struct basic_block, bb_list_node);
}

static inline bool bb_mimic_stack_is_resolved(struct basic_block *bb)
{
	return bb->entry_mimic_stack_size != -1;
}

struct basic_block *alloc_basic_block(struct compilation_unit *, unsigned long, unsigned long);
struct basic_block *do_alloc_basic_block(struct compilation_unit *, unsigned long, unsigned long);
struct basic_block *get_basic_block(struct compilation_unit *, unsigned long, unsigned long);
void shrink_basic_block(struct basic_block *);
void free_basic_block(struct basic_block *);
struct basic_block *bb_split(struct basic_block *, unsigned long);
void bb_add_stmt(struct basic_block *, struct statement *);
void bb_add_insn(struct basic_block *, struct insn *);
void bb_add_first_insn(struct basic_block *, struct insn *);
struct insn *bb_first_insn(struct basic_block *);
struct insn *bb_last_insn(struct basic_block *);
int bb_add_successor(struct basic_block *, struct basic_block *);
struct basic_block *ssa_insert_chg_bb(struct compilation_unit *, struct basic_block *,
				struct basic_block *, unsigned int);
struct basic_block *ssa_insert_empty_bb(struct compilation_unit *, struct basic_block *,
				struct basic_block *, unsigned int);
bool bb_successors_contains(struct basic_block *, struct basic_block *);
int bb_add_mimic_stack_expr(struct basic_block *, struct expression *);
struct statement *bb_remove_last_stmt(struct basic_block *bb);
unsigned char *bb_native_ptr(struct basic_block *bb);
void resolution_block_init(struct resolution_block *block);
bool branch_needs_resolution_block(struct basic_block *from, int idx);
int bb_lookup_successor_index(struct basic_block *from, struct basic_block *to);
void clear_mimic_stack(struct stack *);

#define for_each_basic_block(bb, bb_list) list_for_each_entry(bb, bb_list, bb_list_node)
#define for_each_basic_block_reverse(bb, bb_list) list_for_each_entry_reverse(bb, bb_list, bb_list_node)

#endif
