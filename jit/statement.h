#ifndef __STATEMENT_H
#define __STATEMENT_H

#include <list.h>
#include <expression.h>
#include <stddef.h>
#include <jam.h>

enum statement_type {
	STMT_NOP,
	STMT_STORE,
	STMT_IF,
	STMT_LABEL,
	STMT_GOTO,
	STMT_RETURN,
	STMT_EXPRESSION,
	STMT_NULL_CHECK,
	STMT_ARRAY_CHECK,
};

struct statement {
	enum statement_type type;
	union {
		/* STMT_NOP and STMT_LABEL have no fields.  */
		
		struct /* STMT_STORE */ {
			struct expression *store_dest;
			struct expression *store_src;
		};
		struct /* STMT_IF */ {
			struct expression *if_conditional;
			struct statement *if_true;
		};
		struct /* STMT_GOTO */ {
			struct statement *goto_target;
		};
		struct /* STMT_RETURN */ {
			struct expression *return_value;
		};
		/* STMT_EXPRESSION, STMT_NULL_CHECK, STMT_ARRAY_CHECK */
		struct expression *expression;
	};
	struct list_head stmt_list_node;
};

struct statement *alloc_statement(enum statement_type);
void free_statement(struct statement *);

#endif
