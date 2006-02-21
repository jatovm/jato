/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <expression.h>
#include <statement.h>
#include <stdlib.h>
#include <string.h>

struct statement *alloc_statement(enum statement_type type)
{
	struct statement *stmt = malloc(sizeof *stmt);
	if (!stmt)
		return NULL;
	memset(stmt, 0, sizeof *stmt);
	stmt->type = type;

	return stmt;
}

void free_statement(struct statement *stmt)
{
	if (!stmt)
		return;

	free_statement(stmt->next);
	switch (stmt->type) {
	case STMT_NOP:
		/* nothing to do */
		break;
	case STMT_ASSIGN:
		expr_put(stmt->left);
		expr_put(stmt->right);
		break;
	case STMT_IF:
		expr_put(stmt->if_conditional);
		break;
	case STMT_LABEL:
	case STMT_GOTO:
		break;
	case STMT_NULL_CHECK:
	case STMT_ARRAY_CHECK:
		expr_put(stmt->expression);
		break;
	};
	free(stmt);
}
