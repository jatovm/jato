/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <statement.h>

struct statement *alloc_stmt(enum statement_type type)
{
	struct statement *stmt = malloc(sizeof(*stmt));
	if (!stmt)
		return NULL;

	stmt->s_type = type;

	stmt->s_left = malloc(sizeof(struct operand));
	if (!stmt->s_left)
		goto failed;

	stmt->s_right = malloc(sizeof(struct operand));
	if (!stmt->s_right)
		goto failed;

	return stmt;

  failed:
	free(stmt->s_left);
	free(stmt->s_right);
	free(stmt);
	return NULL;
}

void free_stmt(struct statement *stmt)
{
	free(stmt);
}
