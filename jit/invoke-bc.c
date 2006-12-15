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
#include <jit/jit-compiler.h>
#include <jit/statement.h>
#include <vm/bytecode.h>
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
	bb_add_stmt(bb, return_stmt);
	return 0;
}

int convert_void_return(struct compilation_unit *cu,
			struct basic_block *bb, unsigned long offset)
{
	struct statement *return_stmt = alloc_statement(STMT_VOID_RETURN);
	if (!return_stmt)
		return -ENOMEM;

	return_stmt->return_value = NULL;

	bb_add_stmt(bb, return_stmt);
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

static int convert_and_add_args(struct compilation_unit *cu,
				struct methodblock *invoke_target,
				struct expression *expr)
{
	struct expression *args_list;

	args_list = convert_args(cu->expr_stack, invoke_target->args_count);
	if (!args_list)
		return -ENOMEM;

	expr->args_list = &args_list->node;

	return 0;
}

static int insert_invoke_expr(struct compilation_unit *cu, struct basic_block *bb,
			    struct expression *invoke_expr)
{
	if (invoke_expr->vm_type == J_VOID) {
		struct statement *expr_stmt;

		expr_stmt = alloc_statement(STMT_EXPRESSION);
		if (!expr_stmt)
			return -ENOMEM;

		expr_stmt->expression = &invoke_expr->node;
		bb_add_stmt(bb, expr_stmt);
	} else
		stack_push(cu->expr_stack, invoke_expr);

	return 0;
}

static struct methodblock *resolve_invoke_target(struct methodblock *current,
						 unsigned long offset)
{
	unsigned long idx; 

	idx = read_u16(current->jit_code, offset + 1);
	return resolveMethod(current->class, idx);
}

int convert_invokevirtual(struct compilation_unit *cu,
			  struct basic_block *bb, unsigned long offset)
{
	struct methodblock *invoke_target;
	struct expression *expr;
	int err = -ENOMEM;

	invoke_target = resolve_invoke_target(cu->method, offset);
	if (!invoke_target)
		return -EINVAL;

	expr = invokevirtual_expr(invoke_target);
	if (!expr)
		return -ENOMEM;

	err = convert_and_add_args(cu, invoke_target, expr);
	if (err)
		goto failed;

	err = insert_invoke_expr(cu, bb, expr);
	if (err)
		goto failed;

	return 0;
      failed:
	expr_put(expr);
	return err;
}

int convert_invokespecial(struct compilation_unit *cu,
			  struct basic_block *bb, unsigned long offset)
{
	struct methodblock *invoke_target;
	struct expression *expr;
	int err = -ENOMEM;

	invoke_target = resolve_invoke_target(cu->method, offset);
	if (!invoke_target)
		return -EINVAL;

	expr = invoke_expr(invoke_target);
	if (!expr)
		return -ENOMEM;

	err = convert_and_add_args(cu, invoke_target, expr);
	if (err)
		goto failed;

	err = insert_invoke_expr(cu, bb, expr);
	if (err)
		goto failed;

	return 0;
      failed:
	expr_put(expr);
	return err;
}

int convert_invokestatic(struct compilation_unit *cu,
			 struct basic_block *bb, unsigned long offset)
{
	struct methodblock *invoke_target;
	struct expression *expr;
	int err = -ENOMEM;

	invoke_target = resolve_invoke_target(cu->method, offset);
	if (!invoke_target)
		return -EINVAL;

	expr = invoke_expr(invoke_target);
	if (!expr)
		return -ENOMEM;

	err = convert_and_add_args(cu, invoke_target, expr);
	if (err)
		goto failed;

	err = insert_invoke_expr(cu, bb, expr);
	if (err)
		goto failed;

	return 0;
      failed:
	expr_put(expr);
	return err;
}
