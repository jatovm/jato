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

	return_stmt->return_value = NULL;

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

static struct expression *convert_args(struct stack *expr_stack,
				       unsigned long nr_args)
{
	struct expression *args_list = NULL;
	unsigned long i;

	if (nr_args == 0) {
		args_list = no_args_expr();
		goto out;
	}

	for (i = 0; i < nr_args; i++) {
		struct expression *expr = stack_pop(expr_stack);
		args_list = insert_arg(args_list, expr);
	}

  out:
	return args_list;
}

static int __convert_invoke(struct compilation_unit *cu, struct basic_block *bb,
			    struct expression *invoke_expr)
{
	if (invoke_expr->jvm_type == J_VOID) {
		struct statement *expr_stmt;

		expr_stmt = alloc_statement(STMT_EXPRESSION);
		if (!expr_stmt)
			return -ENOMEM;

		expr_stmt->expression = &invoke_expr->node;
		bb_insert_stmt(bb, expr_stmt);
	} else
		stack_push(cu->expr_stack, invoke_expr);

	return 0;
}

static enum jvm_type method_return_type(struct methodblock *method)
{
	char *return_type = method->type + (strlen(method->type) - 1);
	return str_to_type(return_type);
}

int convert_invokevirtual(struct compilation_unit *cu,
			  struct basic_block *bb, unsigned long offset)
{
	int err = -ENOMEM;
	unsigned long method_index;
	struct methodblock *target_method;
	enum jvm_type return_type;
	struct expression *invoke_expr;
	struct expression *args_list;

	method_index = cp_index(cu->method->code + offset + 1);

	target_method = resolveMethod(cu->method->class, method_index);
	if (!target_method)
		return -EINVAL;

	return_type = method_return_type(target_method);
	invoke_expr = invokevirtual_expr(return_type, method_index);
	if(!invoke_expr)
		return -ENOMEM;

	args_list = convert_args(cu->expr_stack, target_method->args_count);
	if (!args_list)
		goto failed;

	invoke_expr->args_list = &args_list->node;

	err = __convert_invoke(cu, bb, invoke_expr);
	if (err)
		goto failed;

	return 0;
      failed:
	expr_put(invoke_expr);
	return err;
}

int convert_invokestatic(struct compilation_unit *cu,
			 struct basic_block *bb, unsigned long offset)
{
	int err = -ENOMEM;
	unsigned long method_index;
	struct methodblock *target_method;
	enum jvm_type return_type;
	struct expression *inv_expr;
	struct expression *args_list;

	method_index = cp_index(cu->method->code + offset + 1);

	target_method = resolveMethod(cu->method->class, method_index);
	if (!target_method)
		return -EINVAL;

	return_type = method_return_type(target_method);
	inv_expr = invoke_expr(return_type, target_method);
	if (!inv_expr)
		return -ENOMEM;

	args_list = convert_args(cu->expr_stack, target_method->args_count);
	if (!args_list)
		goto failed;

	inv_expr->args_list = &args_list->node;

	err = __convert_invoke(cu, bb, inv_expr);
	if (err)
		goto failed;

	return 0;
      failed:
	expr_put(inv_expr);
	return err;
}
