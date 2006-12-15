/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode arithmetic
 * instructions to immediate representation of the JIT compiler.
 */

#include <bytecodes.h>
#include <errno.h>

#include <jit/jit-compiler.h>
#include <jit/statement.h>

#include <vm/bytecode.h>
#include <vm/field.h>
#include <vm/stack.h>

static struct fieldblock *lookup_field(struct compilation_unit *cu, unsigned long offset)
{
	unsigned short index;

	index = read_u16(cu->method->jit_code, offset + 1);
	return resolveField(cu->method->class, index);
}

static int __convert_field_get(enum expression_type expr_type,
			       struct compilation_unit *cu,
			       unsigned long offset)
{
	struct expression *value;
	struct fieldblock *fb;

	fb = lookup_field(cu, offset);
	if (!fb)
		return -EINVAL;

	value = __field_expr(expr_type, field_type(fb), fb);
	if (!value)
		return -ENOMEM;

	stack_push(cu->expr_stack, value);
	return 0;
}

int convert_getstatic(struct compilation_unit *cu, struct basic_block *bb,
		      unsigned long offset)
{
	return __convert_field_get(EXPR_CLASS_FIELD, cu, offset);
}

int convert_putstatic(struct compilation_unit *cu, struct basic_block *bb,
		      unsigned long offset)
{
	struct fieldblock *fb;
	unsigned short index;
	struct statement *store_stmt;
	struct expression *dest, *src;

	index = read_u16(cu->method->jit_code, offset + 1);

	fb = resolveField(cu->method->class, index);
	if (!fb)
		return -EINVAL;

	src = stack_pop(cu->expr_stack);
	dest = class_field_expr(field_type(fb), fb);
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

int convert_getfield(struct compilation_unit *cu, struct basic_block *bb,
		      unsigned long offset)
{
	return __convert_field_get(EXPR_INSTANCE_FIELD, cu, offset);
}

static unsigned long alloc_temporary(void)
{
	static unsigned long temporary;
	return ++temporary;
}

int convert_array_load(struct compilation_unit *cu,
			      struct basic_block *bb, enum vm_type type)
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
			       struct basic_block *bb, enum vm_type type)
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

int convert_new(struct compilation_unit *cu, struct basic_block *bb,
		unsigned long offset)
{
	struct expression *expr;
	unsigned long type_idx;
	struct object *class;

	type_idx = read_u16(cu->method->jit_code, offset + 1);
	class = resolveClass(cu->method->class, type_idx, FALSE);
	if (!class)
		return -EINVAL;

	expr = new_expr(class);
	if (!expr)
		return -ENOMEM;

	stack_push(cu->expr_stack, expr);

	return 0;
}
