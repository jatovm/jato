#ifndef __STATEMENT_H
#define __STATEMENT_H

#include <vm/list.h>
#include <jit/expression.h>
#include <stddef.h>
#include <vm/vm.h>

enum statement_type {
	STMT_NOP = OP_LAST,
	STMT_STORE,
	STMT_IF,
	STMT_GOTO,
	STMT_RETURN,
	STMT_VOID_RETURN,
	STMT_EXPRESSION,
	STMT_NULL_CHECK,
	STMT_ARRAY_CHECK,
	STMT_LAST,	/* Not a real type. Keep this last.  */
};

struct statement {
	union {
		struct tree_node node;

		/* STMT_NOP and STMT_VOID_RETURN have no fields.  */
		
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
		/* STMT_EXPRESSION, STMT_NULL_CHECK, STMT_ARRAY_CHECK */
		struct tree_node *expression;
	};
	struct list_head stmt_list_node;
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

#endif
