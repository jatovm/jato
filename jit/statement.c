/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <vm/vm.h>
#include <assert.h>
#include <jit/expression.h>
#include <jit/statement.h>
#include <stdlib.h>
#include <string.h>

struct statement *alloc_statement(enum statement_type type)
{
	struct statement *stmt = malloc(sizeof *stmt);
	if (stmt) {
		memset(stmt, 0, sizeof *stmt);
		INIT_LIST_HEAD(&stmt->stmt_list_node);
		stmt->node.op = type << STMT_TYPE_SHIFT;
	}

	return stmt;
}

void free_statement(struct statement *stmt)
{
	if (!stmt)
		return;

	switch (stmt_type(stmt)) {
	case STMT_NOP:
		/* nothing to do */
		break;
	case STMT_STORE:
		expr_put(to_expr(stmt->store_dest));
		expr_put(to_expr(stmt->store_src));
		break;
	case STMT_IF:
		expr_put(to_expr(stmt->if_conditional));
		break;
	case STMT_GOTO:
		break;
	case STMT_RETURN:
		if (stmt->return_value)
			expr_put(to_expr(stmt->return_value));
		break;
	case STMT_VOID_RETURN:
		break;
	case STMT_EXPRESSION:
	case STMT_NULL_CHECK:
	case STMT_ARRAY_CHECK:
		if (stmt->expression)
			expr_put(to_expr(stmt->expression));
		break;
	case STMT_LAST:
		assert(!"STMT_LAST is not a real type. Don't use it!");
		break;
	};
	free(stmt);
}
