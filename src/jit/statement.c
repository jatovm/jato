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
	stmt->s_type = type;

	return stmt;
}

void free_stmt(struct statement *stmt)
{
	if (stmt) {
		free_stmt(stmt->s_next);
		if (stmt->s_target)
			expr_put(stmt->s_target);
		if (stmt->s_left)
			expr_put(stmt->s_left);
		if (stmt->s_right)
			expr_put(stmt->s_right);
		free(stmt);
	}
}
