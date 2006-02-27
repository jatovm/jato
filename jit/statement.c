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
	if (stmt) {
		memset(stmt, 0, sizeof *stmt);
		INIT_LIST_HEAD(&stmt->stmt_list_node);
		stmt->type = type;
	}

	return stmt;
}

void free_statement(struct statement *stmt)
{
	if (!stmt)
		return;

	switch (stmt->type) {
	case STMT_NOP:
		/* nothing to do */
		break;
	case STMT_STORE:
		expr_put(stmt->store_dest);
		expr_put(stmt->store_src);
		break;
	case STMT_IF:
		expr_put(stmt->if_conditional);
		break;
	case STMT_LABEL:
	case STMT_GOTO:
		break;
	case STMT_RETURN:
		if (stmt->return_value)
			expr_put(stmt->return_value);
		break;
	case STMT_NULL_CHECK:
	case STMT_ARRAY_CHECK:
		expr_put(stmt->expression);
		break;
	};
	free(stmt);
}
