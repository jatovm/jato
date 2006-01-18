/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <statement.h>
#include <stdlib.h>

struct statement *alloc_stmt(enum statement_type type)
{
	struct statement *stmt = malloc(sizeof(*stmt));
	if (!stmt)
		return NULL;
	memset(stmt, 0, sizeof(*stmt));

	stmt->s_type = type;

	stmt->s_left = malloc(sizeof(struct operand));
	if (!stmt->s_left)
		goto failed;

	stmt->s_right = malloc(sizeof(struct operand));
	if (!stmt->s_right)
		goto failed;

	stmt->s_target = malloc(sizeof(struct operand));
	if (!stmt->s_target)
		goto failed;

	return stmt;

  failed:
	free_stmt(stmt);
	return NULL;
}

void free_stmt(struct statement *stmt)
{
	if (stmt) {
		free_stmt(stmt->s_next);
		free(stmt->s_target);
		free(stmt->s_left);
		free(stmt->s_right);
		free(stmt);
	}
}
