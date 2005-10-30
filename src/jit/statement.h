#ifndef __STATEMENT_H
#define __STATEMENT_H

#include <operand.h>
#include <stddef.h>
#include <jam.h>

struct operand_stack;

enum statement_type {
	STMT_NOP,
	STMT_ASSIGN,
	STMT_NULL_CHECK,
	STMT_ARRAY_CHECK,
};

struct statement {
	enum statement_type type;	/* Type of the statement.  */
	unsigned long target;		/* Target temporary of the statement.  */
	struct operand s_left;		/* Left operand of the statement.  */
	struct operand s_right;		/* Left operand of the statement.  */
	struct statement *next;		/* Next statement. */
};

extern struct statement *stmt_from_bytecode(struct classblock *,
					    unsigned char *,
					    size_t, struct operand_stack *);

#endif
