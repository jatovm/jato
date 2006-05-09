/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode method invocation
 * and return instructions to immediate representation of the JIT compiler.
 */

#include <bytecodes.h>
#include <jit-compiler.h>
#include <jit/statement.h>
#include <vm/const-pool.h>
#include <vm/stack.h>

#include <string.h>
#include <errno.h>

int convert_non_void_return(struct compilation_unit *cu,
			    struct basic_block *bb, unsigned long offset)
{
	struct expression *expr;
	struct statement *return_stmt = alloc_statement(STMT_RETURN);
	if (!return_stmt)
		return -ENOMEM;

	expr = stack_pop(cu->expr_stack);
	return_stmt->return_value = &expr->node;
	bb_insert_stmt(bb, return_stmt);
	return 0;
}

int convert_void_return(struct compilation_unit *cu,
			struct basic_block *bb, unsigned long offset)
{
	struct statement *return_stmt = alloc_statement(STMT_VOID_RETURN);
	if (!return_stmt)
		return -ENOMEM;

	bb_insert_stmt(bb, return_stmt);
	return 0;
}

static struct expression *insert_arg(struct expression *root,
				     struct expression *expr)
{
	if (!root)
		return arg_expr(expr);

	return args_list_expr(root, arg_expr(expr));
}

int convert_invokestatic(struct compilation_unit *cu,
			 struct basic_block *bb, unsigned long offset)
{
	struct methodblock *mb;
	unsigned short index;
	struct expression *value, *args_list = NULL;
	char *return_type;
	int i;

	index = cp_index(cu->method->code + offset + 1);

	mb = resolveMethod(cu->method->class, index);
	if (!mb)
		return -EINVAL;

	return_type = mb->type + (strlen(mb->type) - 1);
	value = invoke_expr(str_to_type(return_type), mb);
	if (!value)
		return -ENOMEM;

	if (!mb->args_count)
		args_list = no_args_expr();

	for (i = 0; i < mb->args_count; i++)
		args_list = insert_arg(args_list, stack_pop(cu->expr_stack));

	if (args_list)
		value->args_list = &args_list->node;

	if (value->jvm_type == J_VOID) {
		struct statement *expr_stmt;

		expr_stmt = alloc_statement(STMT_EXPRESSION);
		if (!expr_stmt)
			goto failed;

		expr_stmt->expression = &value->node;
		bb_insert_stmt(bb, expr_stmt);
	} else
		stack_push(cu->expr_stack, value);

	return 0;
      failed:
	expr_put(value);
	return -ENOMEM;
}
