#ifndef __STATEMENT_H
#define __STATEMENT_H

#include <expression.h>
#include <stddef.h>
#include <jam.h>

enum statement_type {
	STMT_NOP,
	STMT_ASSIGN,
	STMT_IF,
	STMT_LABEL,
	STMT_GOTO,
	STMT_NULL_CHECK,
	STMT_ARRAY_CHECK,
};

struct statement {
	enum statement_type type;
	union {
		/* STMT_NOP and STMT_LABEL have no fields.  */
		/* STMT_ASSIGN */
		struct {
			struct expression *left;
			struct expression *right;
		};
		/* STMT_IF */
		struct {
			struct expression *if_conditional;
			struct statement *if_true;
		};
		/* STMT_GOTO */
		struct {
			struct statement *goto_target;
		};
		/* STMT_NULL_CHECK, STMT_ARRAY_CHECK */
		struct expression *expression;
	};
	struct statement *next;
};

struct statement *alloc_statement(enum statement_type);
void free_statement(struct statement *);

#endif
