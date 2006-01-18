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
	enum statement_type s_type;	/* Type of the statement.  */
	struct expression *s_target;	/* Target temporary of the statement.  */
	struct expression *s_left;		/* Left expression of the statement.  */
	struct expression *s_right;	/* Left expression of the statement.  */
	struct statement *s_next;	/* Next statement. */
};

struct statement *convert_bytecode_to_stmts(struct classblock *,
					    unsigned char *, unsigned long,
					    struct stack *);

struct statement *alloc_stmt(enum statement_type);
void free_stmt(struct statement *);

#endif
