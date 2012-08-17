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

#include "jit/statement.h"
#include "jit/compiler.h"
#include "jit/args.h"

#include "vm/bytecode.h"
#include "vm/method.h"
#include "vm/class.h"
#include "vm/die.h"
#include "vm/jni.h"

#include "lib/stack.h"

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
	clear_mimic_stack(ctx->bb->mimic_stack);
	return 0;
}

int convert_return(struct parse_context *ctx)
{
	struct statement *return_stmt = alloc_statement(STMT_VOID_RETURN);
	if (!return_stmt)
		return warn("out of memory"), -ENOMEM;

	return_stmt->return_value = NULL;

	convert_statement(ctx, return_stmt);
	clear_mimic_stack(ctx->bb->mimic_stack);
	return 0;
}

static int convert_and_add_args(struct parse_context *ctx,
				struct vm_method *invoke_target,
				struct statement *stmt)
{
	struct expression **args_array;
	struct expression *args_list;
	unsigned long nr_args, i;

	nr_args = vm_method_arg_stack_count(invoke_target);

	args_array = pop_args(ctx->bb->mimic_stack, nr_args);
	if (!args_array)
		return warn("out of memory"), -EINVAL;

	for (i = 0; i < nr_args; i++) {
		struct expression *expr = args_array[i];

		if (!expr_is_pure(expr))
			args_array[i] = get_pure_expr(ctx, expr);

		if (vm_type_is_pair(expr->vm_type))
			i++;

		if (i >= nr_args)
			break;
	}

	args_list = convert_args(args_array, nr_args, invoke_target);
	if (!args_list) {
		free(args_array);
		return warn("out of memory"), -EINVAL;
	}

	free(args_array);

	stmt->args_list = &args_list->node;

	return 0;
}

static struct statement * invoke_stmt(struct parse_context *ctx,
				      enum statement_type type,
				      struct vm_method *target)
{
	struct statement *stmt;
	enum vm_type return_type;

	assert(type == STMT_INVOKE || type == STMT_INVOKEVIRTUAL ||
	       type == STMT_INVOKEINTERFACE);

	stmt = alloc_statement(type);
	if (!stmt)
		return NULL;

	stmt->target_method = target;

	return_type = method_return_type(target);
	switch (return_type) {
	case J_VOID:
		stmt->invoke_result = NULL;
		break;
	case J_LONG:
	case J_FLOAT:
	case J_DOUBLE:
		stmt->invoke_result = temporary_expr(return_type, ctx->cu);
		break;
	case J_BYTE:
	case J_BOOLEAN:
	case J_CHAR:
	case J_INT:
	case J_SHORT:
		stmt->invoke_result = temporary_expr(J_INT, ctx->cu);
		break;
	case J_REFERENCE:
		stmt->invoke_result = temporary_expr(J_REFERENCE, ctx->cu);
		break;
	default:
		error("invalid method return type %d", return_type);
	};

	return stmt;
}

static void insert_invoke_stmt(struct parse_context *ctx, struct statement *stmt)
{
	convert_statement(ctx, stmt);

	if (stmt->invoke_result) {
		expr_get(stmt->invoke_result);
		convert_expression(ctx, stmt->invoke_result);
	}
}

static struct vm_method *resolve_invoke_target(struct parse_context *ctx, uint16_t access_flags)
{
	unsigned long idx;

	idx = bytecode_read_u16(ctx->buffer);

	return vm_class_resolve_method_recursive(ctx->cu->method->class, idx, access_flags);
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
	struct statement *stmt;
	int count;
	int zero;
	int err;

	invoke_target = resolve_invokeinterface_target(ctx);
	if (!invoke_target)
		return warn("unable to resolve invocation target"), -EINVAL;

	stmt = invoke_stmt(ctx, STMT_INVOKEINTERFACE, invoke_target);
	if (!stmt)
		return warn("out of memory"), -ENOMEM;

	count = bytecode_read_u8(ctx->buffer);
	if (count == 0)
		return warn("invokeinterface count must not be zero"), -EINVAL;

	zero = bytecode_read_u8(ctx->buffer);
	if (zero != 0) {
		return warn("invokeinterface fourth operand byte not zero"),
			-EINVAL;
	}

	err = convert_and_add_args(ctx, invoke_target, stmt);
	if (err)
		goto failed;

	insert_invoke_stmt(ctx, stmt);
	return 0;

failed:
	free_statement(stmt);
	return err;
}

int convert_invokevirtual(struct parse_context *ctx)
{
	struct vm_method *invoke_target;
	struct statement *stmt;
	int err = -ENOMEM;

	invoke_target = resolve_invoke_target(ctx, 0);
	if (!invoke_target)
		return warn("unable to resolve invocation target"), -EINVAL;

	stmt = invoke_stmt(ctx, STMT_INVOKEVIRTUAL, invoke_target);
	if (!stmt)
		return warn("out of memory"), -ENOMEM;

	err = convert_and_add_args(ctx, invoke_target, stmt);
	if (err)
		goto failed;

	insert_invoke_stmt(ctx, stmt);
	return 0;
      failed:
	free_statement(stmt);
	return err;
}

int convert_invokespecial(struct parse_context *ctx)
{
	struct vm_method *invoke_target;
	struct statement *stmt;
	int err;

	invoke_target = resolve_invoke_target(ctx, CAFEBABE_CLASS_ACC_STATIC);
	if (!invoke_target)
		return warn("unable to resolve invocation target"), -EINVAL;

	stmt = invoke_stmt(ctx, STMT_INVOKE, invoke_target);
	if (!stmt)
		return warn("out of memory"), -ENOMEM;

	err = convert_and_add_args(ctx, invoke_target, stmt);
	if (err)
		goto failed;

	null_check_this_arg(to_expr(stmt->args_list));

	insert_invoke_stmt(ctx, stmt);
	return 0;
      failed:
	free_statement(stmt);
	return err;
}

int convert_invokestatic(struct parse_context *ctx)
{
	struct vm_method *invoke_target;
	struct statement *stmt;
	int err;

	invoke_target = resolve_invoke_target(ctx, CAFEBABE_CLASS_ACC_STATIC);
	if (!invoke_target)
		return warn("unable to resolve invocation target"), -EINVAL;

	stmt = invoke_stmt(ctx, STMT_INVOKE, invoke_target);
	if (!stmt)
		return warn("out of memory"), -ENOMEM;

	err = convert_and_add_args(ctx, invoke_target, stmt);
	if (err)
		goto failed;

	insert_invoke_stmt(ctx, stmt);
	return 0;
      failed:
	free_statement(stmt);
	return err;
}
