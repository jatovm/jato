#ifndef __STATEMENT_H
#define __STATEMENT_H

#include <expression.h>
#include <stddef.h>
#include <jam.h>

struct stack;

enum statement_type {
	STMT_NOP,
	STMT_ASSIGN,
	STMT_NULL_CHECK,
	STMT_ARRAY_CHECK,
};

struct statement {
	enum statement_type type;
	union {
		/* STMT_ASSIGN */
		struct {
			struct expression *left;
			struct expression *right;
		};
		/* STMT_NULL_CHECK, STMT_ARRAY_CHECK */
		struct expression *expression;
	};
	struct statement *next;
};

struct statement *convert_bytecode_to_stmts(struct classblock *,
					    unsigned char *, unsigned long,
					    struct stack *);

struct statement *alloc_statement(enum statement_type);
void free_statement(struct statement *);

#endif
