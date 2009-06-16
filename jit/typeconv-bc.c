/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode type conversion
 * instructions to immediate representation of the JIT compiler.
 */

#include <jit/compiler.h>
#include <jit/statement.h>

#include <vm/bytecodes.h>
#include <vm/stack.h>

#include <errno.h>

static int convert_conversion(struct parse_context *ctx, enum vm_type to_type)
{
	struct expression *from_expression, *conversion_expression;

	from_expression = stack_pop(ctx->bb->mimic_stack);

	conversion_expression = conversion_expr(to_type, from_expression);
	if (!conversion_expression)
		return -ENOMEM;

	convert_expression(ctx, conversion_expression);
	return 0;
}

int convert_i2l(struct parse_context *ctx)
{
	return convert_conversion(ctx, J_LONG);
}

int convert_i2f(struct parse_context *ctx)
{
	return convert_conversion(ctx, J_FLOAT);
}

int convert_i2d(struct parse_context *ctx)
{
	return convert_conversion(ctx, J_DOUBLE);
}

int convert_l2i(struct parse_context *ctx)
{
	return convert_conversion(ctx, J_INT);
}

int convert_l2f(struct parse_context *ctx)
{
	return convert_conversion(ctx, J_FLOAT);
}

int convert_l2d(struct parse_context *ctx)
{
	return convert_conversion(ctx, J_DOUBLE);
}

int convert_f2i(struct parse_context *ctx)
{
	return convert_conversion(ctx, J_INT);
}

int convert_f2l(struct parse_context *ctx)
{
	return convert_conversion(ctx, J_LONG);
}

int convert_f2d(struct parse_context *ctx)
{
	return convert_conversion(ctx, J_DOUBLE);
}

int convert_d2i(struct parse_context *ctx)
{
	return convert_conversion(ctx, J_INT);
}

int convert_d2l(struct parse_context *ctx)
{
	return convert_conversion(ctx, J_LONG);
}

int convert_d2f(struct parse_context *ctx)
{
	return convert_conversion(ctx, J_FLOAT);
}

int convert_i2b(struct parse_context *ctx)
{
	return convert_conversion(ctx, J_BYTE);
}

int convert_i2c(struct parse_context *ctx)
{
	return convert_conversion(ctx, J_CHAR);
}

int convert_i2s(struct parse_context *ctx)
{
	return convert_conversion(ctx, J_SHORT);
}
