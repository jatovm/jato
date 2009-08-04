/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include "vm/bytecode.h"
#include "vm/bytecodes.h"
#include "vm/vm.h"
#include <assert.h>
#include "jit/expression.h"
#include "jit/statement.h"
#include "jit/bc-offset-mapping.h"
#include <stdlib.h>
#include <string.h>

/* How many child expression nodes are used by each type of statement. */
int stmt_nr_kids(struct statement *stmt)
{
	switch (stmt_type(stmt)) {
	case STMT_STORE:
	case STMT_ARRAY_STORE_CHECK:
		return 2;
	case STMT_IF:
	case STMT_RETURN:
	case STMT_EXPRESSION:
	case STMT_ARRAY_CHECK:
	case STMT_ATHROW:
	case STMT_MONITOR_ENTER:
	case STMT_MONITOR_EXIT:
	case STMT_CHECKCAST:
	case STMT_TABLESWITCH:
		return 1;
	case STMT_GOTO:
	case STMT_VOID_RETURN:
		return 0;
	default:
		assert(!"Invalid statement type");
	}
}

struct statement *alloc_statement(enum statement_type type)
{
	struct statement *stmt = malloc(sizeof *stmt);
	if (stmt) {
		memset(stmt, 0, sizeof *stmt);
		INIT_LIST_HEAD(&stmt->stmt_list_node);
		stmt->node.op = type << STMT_TYPE_SHIFT;
		stmt->node.bytecode_offset = BC_OFFSET_UNKNOWN;
	}

	return stmt;
}

void free_statement(struct statement *stmt)
{
	int i;

	if (!stmt)
		return;

	for (i = 0; i < stmt_nr_kids(stmt); i++)
		if (stmt->node.kids[i])
			expr_put(to_expr(stmt->node.kids[i]));

	free(stmt);
}

struct tableswitch *alloc_tableswitch(struct tableswitch_info *info,
				      struct compilation_unit *cu,
				      unsigned long offset)
{
	struct tableswitch *table;

	table = malloc(sizeof(*table));
	if (!table)
		return NULL;

	table->low = info->low;
	table->high = info->high;

	table->bb_lookup_table = malloc(sizeof(void *) * info->count);
	if (!table->bb_lookup_table) {
		free(table);
		return NULL;
	}

	for (unsigned int i = 0; i < info->count; i++) {
		int32_t target;

		target = read_s32(info->targets + i * 4);
		table->bb_lookup_table[i] = find_bb(cu, offset + target);
	}

	list_add(&table->list_node, &cu->tableswitch_list);

	return table;
}

void free_tableswitch(struct tableswitch *table)
{
	free(table->lookup_table);
	free(table);
}
