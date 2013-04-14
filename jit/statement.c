/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include "jit/statement.h"

#include "jit/bc-offset-mapping.h"
#include "jit/expression.h"

#include "vm/bytecode.h"
#include "vm/vm.h"

#include <assert.h>
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
	case STMT_LOOKUPSWITCH_JUMP:
	case STMT_INVOKE:
	case STMT_INVOKEINTERFACE:
	case STMT_INVOKEVIRTUAL:
		return 1;
	case STMT_BEFORE_ARGS:
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

	switch (stmt_type(stmt)) {
	case STMT_INVOKE:
	case STMT_INVOKEVIRTUAL:
	case STMT_INVOKEINTERFACE:
		if (stmt->invoke_result)
			expr_put(stmt->invoke_result);
		break;
	default:
		break;
	}

	free(stmt);
}

struct tableswitch *alloc_tableswitch(struct tableswitch_info *info,
				      struct compilation_unit *cu,
				      struct basic_block *bb,
				      unsigned long offset)
{
	struct tableswitch *table;

	table = malloc(sizeof(*table));
	if (!table)
		return NULL;

	table->src = bb;

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

struct lookupswitch *alloc_lookupswitch(struct lookupswitch_info *info,
				       struct compilation_unit *cu,
				       struct basic_block *bb,
				       unsigned long offset)
{
	struct lookupswitch *table;

	table = malloc(sizeof(*table));
	if (!table)
		return NULL;

	table->src = bb;

	table->count = info->count;

	table->pairs = malloc(sizeof(struct lookupswitch_pair) * info->count);
	if (!table->pairs) {
		free(table);
		return NULL;
	}

	for (unsigned int i = 0; i < info->count; i++) {
		int32_t target;

		target = read_lookupswitch_target(info, i);
		table->pairs[i].match = read_lookupswitch_match(info, i);
		table->pairs[i].bb_target = find_bb(cu, offset + target);
	}

	list_add(&table->list_node, &cu->lookupswitch_list);

	return table;
}

void free_lookupswitch(struct lookupswitch *table)
{
	free(table->pairs);
	free(table);
}

int lookupswitch_pair_comp(const void *key, const void *elem)
{
	const struct lookupswitch_pair *pair = elem;

	return (int32_t) ((intptr_t) key) - pair->match;
}
