/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode operand stack
 * management instructions to immediate representation of the JIT compiler.
 */

#include <statement.h>
#include <byteorder.h>
#include <stack.h>
#include <jit-compiler.h>
#include <bytecodes.h>
#include <errno.h>

int convert_pop(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	struct expression *expr = stack_pop(cu->expr_stack);
	
	if (expr_type(expr) == EXPR_INVOKE) {
		struct statement *expr_stmt = alloc_statement(STMT_EXPRESSION);
		if (!expr_stmt)
			return -ENOMEM;
			
		expr_stmt->expression = &expr->node;
		bb_insert_stmt(bb, expr_stmt);
	}
	return 0;
}

int convert_dup(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	void *v;

	v = stack_pop(cu->expr_stack);
	stack_push(cu->expr_stack, v);
	stack_push(cu->expr_stack, v);

	return 0;
}

int convert_dup_x1(struct compilation_unit *cu, struct basic_block *bb,
		   unsigned long offset)
{
	void *v1, *v2;

	v1 = stack_pop(cu->expr_stack);
	v2 = stack_pop(cu->expr_stack);

	stack_push(cu->expr_stack, v1);
	stack_push(cu->expr_stack, v2);
	stack_push(cu->expr_stack, v1);

	return 0;
}

int convert_dup_x2(struct compilation_unit *cu, struct basic_block *bb,
		   unsigned long offset)
{
	void *v1, *v2, *v3;

	v1 = stack_pop(cu->expr_stack);
	v2 = stack_pop(cu->expr_stack);
	v3 = stack_pop(cu->expr_stack);

	stack_push(cu->expr_stack, v1);
	stack_push(cu->expr_stack, v3);
	stack_push(cu->expr_stack, v2);
	stack_push(cu->expr_stack, v1);

	return 0;
}

int convert_swap(struct compilation_unit *cu, struct basic_block *bb,
		 unsigned long offset)
{
	void *v1, *v2;

	v1 = stack_pop(cu->expr_stack);
	v2 = stack_pop(cu->expr_stack);

	stack_push(cu->expr_stack, v1);
	stack_push(cu->expr_stack, v2);

	return 0;
}
