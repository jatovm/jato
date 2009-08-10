/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode method invocation
 * and return instructions to immediate representation of the JIT compiler.
 */

#include "jit/bytecode-to-ir.h"
#include "jit/compiler.h"
#include "jit/statement.h"
#include "jit/args.h"

#include "vm/bytecode.h"
#include "vm/bytecodes.h"
#include "vm/class.h"
#include "vm/method.h"
#include "vm/stack.h"
#include "vm/die.h"
#include "vm/jni.h"

#include <string.h>
#include <errno.h>
#include <stdio.h>

int convert_xreturn(struct parse_context *ctx)
{
	struct expression *expr;
	struct statement *return_stmt = alloc_statement(STMT_RETURN);
	if (!return_stmt)
		return warn("out of memory"), -ENOMEM;

	expr = stack_pop(ctx->bb->mimic_stack);
	return_stmt->return_value = &expr->node;
	convert_statement(ctx, return_stmt);
	return 0;
}

int convert_return(struct parse_context *ctx)
{
	struct statement *return_stmt = alloc_statement(STMT_VOID_RETURN);
	if (!return_stmt)
		return warn("out of memory"), -ENOMEM;

	return_stmt->return_value = NULL;

	convert_statement(ctx, return_stmt);
	return 0;
}

static unsigned int method_real_argument_count(struct vm_method *invoke_target)
{
	int argc;

	argc = count_java_arguments(invoke_target->type);

	if (!vm_method_is_static(invoke_target))
		argc++;

	return argc;
}

static int convert_and_add_args(struct parse_context *ctx,
				struct vm_method *invoke_target,
				struct expression *expr)
{
	struct expression *args_list;

	args_list = convert_args(ctx->bb->mimic_stack,
				 method_real_argument_count(invoke_target),
				 invoke_target);
	if (!args_list)
		return warn("out of memory"), -ENOMEM;

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
			return warn("out of memory"), -ENOMEM;

		expr_stmt->expression = &invoke_expr->node;
		convert_statement(ctx, expr_stmt);
	} else
		convert_expression(ctx, invoke_expr);

	return 0;
}

static struct vm_method *resolve_invoke_target(struct parse_context *ctx)
{
	unsigned long idx;

	idx = bytecode_read_u16(ctx->buffer);

	return vm_class_resolve_method_recursive(ctx->cu->method->class, idx);
}

static struct vm_method *resolve_invokeinterface_target(struct parse_context *ctx)
{
	unsigned long idx;

	idx = bytecode_read_u16(ctx->buffer);

	return vm_class_resolve_interface_method_recursive(ctx->cu->method->class, idx);
}

static void null_check_arg(struct expression *arg)
{
	struct expression *expr;

	expr = null_check_expr(to_expr(arg->arg_expression));
	arg->arg_expression = &expr->node;
}

/**
 * Searches the @arg list for EXPR_ARG_THIS and encapsulates its
 * arg_expression in null check expression.
 */
static void null_check_this_arg(struct expression *arg)
{
	if (expr_type(arg) != EXPR_ARGS_LIST) {
		if (expr_type(arg) == EXPR_ARG_THIS)
			null_check_arg(arg);

		return;
	}

	struct expression *right_arg = to_expr(arg->args_right);
	if (expr_type(right_arg) == EXPR_ARG_THIS) {
		null_check_arg(right_arg);
		return;
	}

	null_check_this_arg(to_expr(arg->args_left));
}

int convert_invokeinterface(struct parse_context *ctx)
{
	struct vm_method *invoke_target;
	struct expression *expr;
	int count;
	int zero;
	int err;

	invoke_target = resolve_invokeinterface_target(ctx);
	if (!invoke_target)
		return warn("unable to resolve invocation target"), -EINVAL;

	expr = invokeinterface_expr(invoke_target);
	if (!expr)
		return warn("out of memory"), -ENOMEM;

	count = bytecode_read_u8(ctx->buffer);
	if (count == 0)
		return warn("invokeinterface count must not be zero"), -EINVAL;

	zero = bytecode_read_u8(ctx->buffer);
	if (zero != 0) {
		return warn("invokeinterface fourth operand byte not zero"),
			-EINVAL;
	}

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

int convert_invokevirtual(struct parse_context *ctx)
{
	struct vm_method *invoke_target;
	struct expression *expr;
	int err = -ENOMEM;

	invoke_target = resolve_invoke_target(ctx);
	if (!invoke_target)
		return warn("unable to resolve invocation target"), -EINVAL;

	if (vm_type_is_float(method_return_type(invoke_target)))
		expr = finvokevirtual_expr(invoke_target);
	else
		expr = invokevirtual_expr(invoke_target);

	if (!expr)
		return warn("out of memory"), -ENOMEM;

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
	struct vm_method *invoke_target;
	struct expression *expr;
	int err;

	invoke_target = resolve_invoke_target(ctx);
	if (!invoke_target)
		return warn("unable to resolve invocation target"), -EINVAL;

	if (vm_type_is_float(method_return_type(invoke_target)))
		expr = finvoke_expr(invoke_target);
	else
		expr = invoke_expr(invoke_target);

	if (!expr)
		return warn("out of memory"), -ENOMEM;

	err = convert_and_add_args(ctx, invoke_target, expr);
	if (err)
		goto failed;

	null_check_this_arg(to_expr(expr->args_list));

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
	struct vm_method *invoke_target;
	struct expression *expr;
	int err;

	invoke_target = resolve_invoke_target(ctx);
	if (!invoke_target)
		return warn("unable to resolve invocation target"), -EINVAL;

	if (vm_type_is_float(method_return_type(invoke_target)))
		expr = finvoke_expr(invoke_target);
	else
		expr = invoke_expr(invoke_target);

	if (!expr)
		return warn("out of memory"), -ENOMEM;

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
