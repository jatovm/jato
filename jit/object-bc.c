/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode arithmetic
 * instructions to immediate representation of the JIT compiler.
 */

#include <jit/statement.h>
#include <vm/stack.h>
#include <jit/jit-compiler.h>
#include <bytecodes.h>
#include <errno.h>
#include <vm/const-pool.h>

int convert_getstatic(struct compilation_unit *cu, struct basic_block *bb,
		      unsigned long offset)
{
	struct fieldblock *fb;
	unsigned short index;
	struct expression *value;

	index = cp_index(cu->method->jit_code + offset + 1);

	fb = resolveField(cu->method->class, index);
	if (!fb)
		return -EINVAL;

	value = field_expr(str_to_type(fb->type), fb);
	if (!value)
		return -ENOMEM;

	stack_push(cu->expr_stack, value);
	return 0;
}

int convert_putstatic(struct compilation_unit *cu, struct basic_block *bb,
		      unsigned long offset)
{
	struct fieldblock *fb;
	unsigned short index;
	struct statement *store_stmt;
	struct expression *dest, *src;

	index = cp_index(cu->method->jit_code + offset + 1);

	fb = resolveField(cu->method->class, index);
	if (!fb)
		return -EINVAL;

	src = stack_pop(cu->expr_stack);
	dest = field_expr(str_to_type(fb->type), fb);
	if (!dest)
		return -ENOMEM;
	
	store_stmt = alloc_statement(STMT_STORE);
	if (!store_stmt) {
		expr_put(dest);
		return -ENOMEM;
	}
	store_stmt->store_dest = &dest->node;
	store_stmt->store_src = &src->node;
	bb_add_stmt(bb, store_stmt);
	
	return 0;
}

static unsigned long alloc_temporary(void)
{
	static unsigned long temporary;
	return ++temporary;
}

int convert_array_load(struct compilation_unit *cu,
			      struct basic_block *bb, enum jvm_type type)
{
	struct expression *index, *arrayref;
	struct expression *src_expr, *dest_expr;
	struct statement *store_stmt, *arraycheck, *nullcheck;

	index = stack_pop(cu->expr_stack);
	arrayref = stack_pop(cu->expr_stack);

	store_stmt = alloc_statement(STMT_STORE);
	if (!store_stmt)
		goto failed;

	src_expr = array_deref_expr(type, arrayref, index);
	dest_expr = temporary_expr(type, alloc_temporary());
	
	store_stmt->store_src = &src_expr->node;
	store_stmt->store_dest = &dest_expr->node;

	expr_get(dest_expr);
	stack_push(cu->expr_stack, dest_expr);

	arraycheck = alloc_statement(STMT_ARRAY_CHECK);
	if (!arraycheck)
		goto failed_arraycheck;

	expr_get(src_expr);
	arraycheck->expression = &src_expr->node;

	nullcheck = alloc_statement(STMT_NULL_CHECK);
	if (!nullcheck)
		goto failed_nullcheck;

	expr_get(arrayref);
	nullcheck->expression = &arrayref->node;

	bb_add_stmt(bb, nullcheck);
	bb_add_stmt(bb, arraycheck);
	bb_add_stmt(bb, store_stmt);

	return 0;

      failed_nullcheck:
	free_statement(arraycheck);
      failed_arraycheck:
	free_statement(store_stmt);
      failed:
	return -ENOMEM;
}

int convert_iaload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_INT);
}

int convert_laload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_LONG);
}

int convert_faload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_FLOAT);
}

int convert_daload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_DOUBLE);
}

int convert_aaload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_REFERENCE);
}

int convert_baload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_INT);
}

int convert_caload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_CHAR);
}

int convert_saload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_SHORT);
}

static int convert_array_store(struct compilation_unit *cu,
			       struct basic_block *bb, enum jvm_type type)
{
	struct expression *value, *index, *arrayref;
	struct statement *store_stmt, *arraycheck, *nullcheck;
	struct expression *src_expr, *dest_expr;

	value = stack_pop(cu->expr_stack);
	index = stack_pop(cu->expr_stack);
	arrayref = stack_pop(cu->expr_stack);

	store_stmt = alloc_statement(STMT_STORE);
	if (!store_stmt)
		goto failed;

	dest_expr = array_deref_expr(type, arrayref, index);
	src_expr = value;

	store_stmt->store_dest = &dest_expr->node;
	store_stmt->store_src = &src_expr->node;

	arraycheck = alloc_statement(STMT_ARRAY_CHECK);
	if (!arraycheck)
		goto failed_arraycheck;

	expr_get(dest_expr);
	arraycheck->expression = &dest_expr->node;

	nullcheck = alloc_statement(STMT_NULL_CHECK);
	if (!nullcheck)
		goto failed_nullcheck;

	expr_get(arrayref);
	nullcheck->expression = &arrayref->node;

	bb_add_stmt(bb, nullcheck);
	bb_add_stmt(bb, arraycheck);
	bb_add_stmt(bb, store_stmt);

	return 0;

      failed_nullcheck:
	free_statement(arraycheck);
      failed_arraycheck:
	free_statement(store_stmt);
      failed:
	return -ENOMEM;
}

int convert_iastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_INT);
}

int convert_lastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_LONG);
}

int convert_fastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_FLOAT);
}

int convert_dastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_DOUBLE);
}

int convert_aastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_REFERENCE);
}

int convert_bastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_INT);
}

int convert_castore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_CHAR);
}

int convert_sastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_SHORT);
}
