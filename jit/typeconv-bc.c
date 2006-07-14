/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode type conversion
 * instructions to immediate representation of the JIT compiler.
 */

#include <jit/statement.h>
#include <vm/stack.h>
#include <jit/jit-compiler.h>
#include <bytecodes.h>
#include <errno.h>

static int convert_conversion(struct compilation_unit *cu,
			      struct basic_block *bb, enum jvm_type to_type)
{
	struct expression *from_expression, *conversion_expression;

	from_expression = stack_pop(cu->expr_stack);

	conversion_expression = conversion_expr(to_type, from_expression);
	if (!conversion_expression)
		return -ENOMEM;

	stack_push(cu->expr_stack, conversion_expression);
	return 0;
}

int convert_i2l(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	return convert_conversion(cu, bb, J_LONG);
}

int convert_i2f(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	return convert_conversion(cu, bb, J_FLOAT);
}

int convert_i2d(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	return convert_conversion(cu, bb, J_DOUBLE);
}

int convert_l2i(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	return convert_conversion(cu, bb, J_INT);
}

int convert_l2f(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	return convert_conversion(cu, bb, J_FLOAT);
}

int convert_l2d(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	return convert_conversion(cu, bb, J_DOUBLE);
}

int convert_f2i(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	return convert_conversion(cu, bb, J_INT);
}

int convert_f2l(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	return convert_conversion(cu, bb, J_LONG);
}

int convert_f2d(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	return convert_conversion(cu, bb, J_DOUBLE);
}

int convert_d2i(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	return convert_conversion(cu, bb, J_INT);
}

int convert_d2l(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	return convert_conversion(cu, bb, J_LONG);
}

int convert_d2f(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	return convert_conversion(cu, bb, J_FLOAT);
}

int convert_i2b(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	return convert_conversion(cu, bb, J_BYTE);
}

int convert_i2c(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	return convert_conversion(cu, bb, J_CHAR);
}

int convert_i2s(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	return convert_conversion(cu, bb, J_SHORT);
}
