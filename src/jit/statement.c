/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <expression.h>
#include <statement.h>
#include <stdlib.h>
#include <string.h>

struct statement *alloc_stmt(enum statement_type type)
{
	struct statement *stmt = malloc(sizeof(*stmt));
	if (!stmt)
		return NULL;
	memset(stmt, 0, sizeof(*stmt));
	stmt->type = type;

	return stmt;
}

void free_stmt(struct statement *stmt)
{
	if (!stmt)
		return;

	free_stmt(stmt->next);
	switch (stmt->type) {
		case STMT_NOP:
			/* nothing to do */
			break;
		case STMT_ASSIGN:
			expr_put(stmt->left);
			expr_put(stmt->right);
			break;
		case STMT_NULL_CHECK:
		case STMT_ARRAY_CHECK:
			expr_put(stmt->expression);
			break;
	};
	free(stmt);
}
