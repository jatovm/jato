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

int convert_xreturn(struct parse_context *ctx)
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

int convert_return(struct parse_context *ctx)
{
	struct statement *return_stmt = alloc_statement(STMT_VOID_RETURN);
	if (!return_stmt)
		return -ENOMEM;

	return_stmt->return_value = NULL;

	convert_statement(ctx, return_stmt);
	return 0;
}

static unsigned int method_real_argument_count(struct vm_method *invoke_target)
{
	unsigned int c = invoke_target->args_count;
	char * a = invoke_target->type;

	/* FIXME: Make more robust, verify correctness. */
	while (*(a++) != ')') {
		if (*a == 'J' || *a == 'D')
			c--;
		if (*a == 'L') {
			while (*(++a) != ';')
				;
		}
	}
	return c;
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

/* Replaces first argument with null check expression on that argument */
static void null_check_first_arg(struct expression *arg)
{
	struct expression *expr;

	if (expr_type(arg) == EXPR_ARG) {
		expr = null_check_expr(to_expr(arg->arg_expression));
		arg->arg_expression = &expr->node;
	}

	if (expr_type(arg) == EXPR_ARGS_LIST)
		null_check_first_arg(to_expr(arg->args_right));
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
		return -ENOMEM;

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

	if (method_return_type(invoke_target) == J_FLOAT)
		expr = finvokevirtual_expr(invoke_target);
	else
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

static void append_arg(struct expression *expr, struct expression *arg)
{
	struct expression *args_list;

	args_list = insert_arg(to_expr(expr->args_list), arg, NULL, NULL);
	expr->args_list = &args_list->node;
}

int convert_invokespecial(struct parse_context *ctx)
{
	struct vm_method *invoke_target;
	struct expression *expr;
	int err;

	invoke_target = resolve_invoke_target(ctx);
	if (!invoke_target)
		return warn("unable to resolve invocation target"), -EINVAL;

	if (method_return_type(invoke_target) == J_FLOAT)
		expr = finvoke_expr(invoke_target);
	else
		expr = invoke_expr(invoke_target);

	if (!expr)
		return -ENOMEM;

	err = convert_and_add_args(ctx, invoke_target, expr);
	if (err)
		goto failed;

	null_check_first_arg(to_expr(expr->args_list));

	if (vm_method_is_jni_method(invoke_target)) {
		struct expression *jni_env_expr;

		jni_env_expr = value_expr(J_REFERENCE,
			(unsigned long)vm_jni_get_jni_env());
		if (!jni_env_expr)
			goto failed;

		append_arg(expr, jni_env_expr);
	}

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

	if (method_return_type(invoke_target) == J_FLOAT)
		expr = finvoke_expr(invoke_target);
	else
		expr = invoke_expr(invoke_target);

	if (!expr)
		return -ENOMEM;

	err = convert_and_add_args(ctx, invoke_target, expr);
	if (err)
		goto failed;

	if (vm_method_is_jni_method(invoke_target)) {
		struct expression *jni_env_expr;
		struct expression *class_expr;

		class_expr = value_expr(J_REFERENCE,
			(unsigned long)invoke_target->class->object);
		if (!class_expr)
			goto failed;

		append_arg(expr, class_expr);

		jni_env_expr = value_expr(J_REFERENCE,
			(unsigned long)vm_jni_get_jni_env());
		if (!jni_env_expr)
			goto failed;

		append_arg(expr, jni_env_expr);
	}

	err = insert_invoke_expr(ctx, expr);
	if (err)
		goto failed;

	return 0;
      failed:
	expr_put(expr);
	return err;
}
