#ifndef __STATEMENT_H
#define __STATEMENT_H

#include "lib/list.h"
#include "jit/expression.h"
#include <stddef.h>
#include "vm/vm.h"

struct tableswitch_info;

enum statement_type {
	STMT_STORE = OP_LAST,
	STMT_IF,
	STMT_GOTO,
	STMT_RETURN,
	STMT_VOID_RETURN,
	STMT_EXPRESSION,
	STMT_ARRAY_CHECK,
	STMT_ATHROW,
	STMT_MONITOR_ENTER,
	STMT_MONITOR_EXIT,
	STMT_CHECKCAST,
	STMT_ARRAY_STORE_CHECK,
	STMT_TABLESWITCH,
	STMT_LAST,	/* Not a real type. Keep this last.  */
};

struct tableswitch {
	uint32_t low;
	uint32_t high;

	union {
		struct basic_block **bb_lookup_table;

		/* Contains native pointers after instruction selection. */
		void **lookup_table;
	};

	struct list_head list_node;
};

struct statement {
	union {
		struct tree_node node;

		/* STMT_VOID_RETURN has no members in this struct.  */

		struct /* STMT_STORE */ {
			struct tree_node *store_dest;
			struct tree_node *store_src;
		};
		struct /* STMT_IF */ {
			struct tree_node *if_conditional;
			struct basic_block *if_true;
		};
		struct /* STMT_GOTO */ {
			struct basic_block *goto_target;
		};
		struct /* STMT_RETURN */ {
			struct tree_node *return_value;
		};
		struct /* STMT_CHECKCAST */ {
			struct tree_node *checkcast_ref;
			struct vm_class *checkcast_class;
		};
		struct /* STMT_ATHROW */ {
			struct tree_node *exception_ref;
		};
		struct /* STMT_ARRAY_STORE_CHECK */ {
			struct tree_node *store_check_src;
			struct tree_node *store_check_array;
		};
		struct /* STMT_TABLESWITCH */ {
			struct tree_node *index;
			struct tableswitch *table;
		};

		/* STMT_EXPRESSION, STMT_ARRAY_CHECK */
		struct tree_node *expression;
	};
	struct list_head stmt_list_node;
	unsigned long bytecode_offset;
};

static inline struct statement *to_stmt(struct tree_node *node)
{
	return container_of(node, struct statement, node);
}

static inline enum statement_type stmt_type(struct statement *stmt)
{
	return (stmt->node.op & STMT_TYPE_MASK) >> STMT_TYPE_SHIFT;
}

struct statement *alloc_statement(enum statement_type);
void free_statement(struct statement *);
int stmt_nr_kids(struct statement *);

struct tableswitch *alloc_tableswitch(struct tableswitch_info *, struct compilation_unit *, unsigned long);
void free_tableswitch(struct tableswitch *);
struct statement *if_stmt(struct basic_block *, enum vm_type, enum binary_operator, struct expression *, struct expression *);

#define for_each_stmt(stmt, stmt_list) list_for_each_entry(stmt, stmt_list, stmt_list_node)

#endif
