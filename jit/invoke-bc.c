/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode method invocation
 * and return instructions to immediate representation of the JIT compiler.
 */

#include <jit/bytecode-converters.h>
#include <jit/compiler.h>
#include <jit/statement.h>

#include <vm/bytecode.h>
#include <vm/bytecodes.h>
#include <vm/stack.h>

#include <string.h>
#include <errno.h>

int convert_non_void_return(struct parse_context *ctx)
{
	struct expression *expr;
	struct statement *return_stmt = alloc_statement(STMT_RETURN);
	if (!return_stmt)
		return -ENOMEM;

	expr = stack_pop(ctx->bb->mimic_stack);
	return_stmt->return_value = &expr->node;
	convert_statement(ctx, return_stmt);
	return 0;
}

int convert_void_return(struct parse_context *ctx)
{
	struct statement *return_stmt = alloc_statement(STMT_VOID_RETURN);
	if (!return_stmt)
		return -ENOMEM;

	return_stmt->return_value = NULL;

	convert_statement(ctx, return_stmt);
	return 0;
}

static unsigned int method_real_argument_count(struct methodblock *invoke_target)
{
	unsigned int c = invoke_target->args_count;
	char * a = invoke_target->type;
	while (*(a++) != ')') {
		if (*a == 'J' || *a == 'D')
			c--;
	}
	return c;
}

static struct expression *insert_arg(struct expression *root,
				     struct expression *expr)
{
	struct expression *_expr;

	_expr = arg_expr(expr);
	_expr->bytecode_offset = expr->bytecode_offset;

	if (!root)
		return _expr;

	return args_list_expr(root, _expr);
}

static struct expression *convert_args(struct stack *mimic_stack,
				       unsigned long nr_args)
{
	struct expression *args_list = NULL;
	unsigned long i;

	if (nr_args == 0) {
		args_list = no_args_expr();
		goto out;
	}

	for (i = 0; i < nr_args; i++) {
		struct expression *expr = stack_pop(mimic_stack);
		args_list = insert_arg(args_list, expr);
	}

  out:
	return args_list;
}

static int convert_and_add_args(struct parse_context *ctx,
				struct methodblock *invoke_target,
				struct expression *expr)
{
	struct expression *args_list;

	args_list = convert_args(ctx->bb->mimic_stack, method_real_argument_count(invoke_target));
	if (!args_list)
		return -ENOMEM;

	expr->args_list = &args_list->node;

	return 0;
}

static int insert_invoke_expr(struct parse_context *ctx,
			      struct expression *invoke_expr)
{
	if (invoke_expr->vm_type == J_VOID) {
		struct statement *expr_stmt;

		expr_stmt = alloc_statement(STMT_EXPRESSION);
		if (!expr_stmt)
			return -ENOMEM;

		expr_stmt->expression = &invoke_expr->node;
		convert_statement(ctx, expr_stmt);
	} else
		convert_expression(ctx, invoke_expr);

	return 0;
}

static struct methodblock *resolve_invoke_target(struct parse_context *ctx)
{
	unsigned long idx;

	idx = bytecode_read_u16(ctx->buffer);

	return resolveMethod(ctx->cu->method->class, idx);
}

int convert_invokevirtual(struct parse_context *ctx)
{
	struct methodblock *invoke_target;
	struct expression *expr;
	int err = -ENOMEM;

	invoke_target = resolve_invoke_target(ctx);
	if (!invoke_target)
		return -EINVAL;

	expr = invokevirtual_expr(invoke_target);
	if (!expr)
		return -ENOMEM;

	err = convert_and_add_args(ctx, invoke_target, expr);
	if (err)
		goto failed;

	err = insert_invoke_expr(ctx, expr);
	if (err)
		goto failed;

	return 0;
      failed:
	expr_put(expr);
	return err;
}

int convert_invokespecial(struct parse_context *ctx)
{
	struct methodblock *invoke_target;
	struct expression *expr;
	int err = -ENOMEM;

	invoke_target = resolve_invoke_target(ctx);
	if (!invoke_target)
		return -EINVAL;

	expr = invoke_expr(invoke_target);
	if (!expr)
		return -ENOMEM;

	err = convert_and_add_args(ctx, invoke_target, expr);
	if (err)
		goto failed;

	err = insert_invoke_expr(ctx, expr);
	if (err)
		goto failed;

	return 0;
      failed:
	expr_put(expr);
	return err;
}

int convert_invokestatic(struct parse_context *ctx)
{
	struct methodblock *invoke_target;
	struct expression *expr;
	int err = -ENOMEM;

	invoke_target = resolve_invoke_target(ctx);
	if (!invoke_target)
		return -EINVAL;

	expr = invoke_expr(invoke_target);
	if (!expr)
		return -ENOMEM;

	err = convert_and_add_args(ctx, invoke_target, expr);
	if (err)
		goto failed;

	err = insert_invoke_expr(ctx, expr);
	if (err)
		goto failed;

	return 0;
      failed:
	expr_put(expr);
	return err;
}
