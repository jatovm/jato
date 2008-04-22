/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode arithmetic
 * instructions to immediate representation of the JIT compiler.
 */

#include <jit/bytecode-converters.h>
#include <jit/compiler.h>
#include <jit/statement.h>

#include <vm/bytecode.h>
#include <vm/bytecodes.h>
#include <vm/field.h>
#include <vm/stack.h>

#include <errno.h>

static struct fieldblock *lookup_field(struct parse_context *ctx)
{
	unsigned short index;

	index = read_u16(ctx->insn_start + 1);
	return resolveField(ctx->cu->method->class, index);
}

int convert_getstatic(struct parse_context *ctx)
{
	struct expression *value;
	struct fieldblock *fb;

	fb = lookup_field(ctx);
	if (!fb)
		return -EINVAL;

	value = class_field_expr(field_type(fb), fb);
	if (!value)
		return -ENOMEM;

	stack_push(ctx->cu->expr_stack, value);
	return 0;
}

int convert_putstatic(struct parse_context *ctx)
{
	struct fieldblock *fb;
	struct statement *store_stmt;
	struct expression *dest, *src;

	fb = lookup_field(ctx);
	if (!fb)
		return -EINVAL;

	src = stack_pop(ctx->cu->expr_stack);
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
	bb_add_stmt(ctx->bb, store_stmt);
	
	return 0;
}

int convert_getfield(struct parse_context *ctx)
{
	struct expression *objectref;
	struct expression *value;
	struct fieldblock *fb;

	fb = lookup_field(ctx);
	if (!fb)
		return -EINVAL;

	objectref = stack_pop(ctx->cu->expr_stack);

	value = instance_field_expr(field_type(fb), fb, objectref);
	if (!value)
		return -ENOMEM;

	stack_push(ctx->cu->expr_stack, value);
	return 0;
}

int convert_putfield(struct parse_context *ctx)
{
	struct expression *dest, *src;
	struct statement *store_stmt;
	struct expression *objectref;
	struct fieldblock *fb;

	fb = lookup_field(ctx);
	if (!fb)
		return -EINVAL;

	src = stack_pop(ctx->cu->expr_stack);
	objectref = stack_pop(ctx->cu->expr_stack);
	dest = instance_field_expr(field_type(fb), fb, objectref);
	if (!dest)
		return -ENOMEM;
	
	store_stmt = alloc_statement(STMT_STORE);
	if (!store_stmt) {
		expr_put(dest);
		return -ENOMEM;
	}
	store_stmt->store_dest = &dest->node;
	store_stmt->store_src = &src->node;
	bb_add_stmt(ctx->bb, store_stmt);
	
	return 0;
}

static unsigned long alloc_temporary(void)
{
	static unsigned long temporary;
	return ++temporary;
}

int convert_array_load(struct parse_context *ctx, enum vm_type type)
{
	struct expression *index, *arrayref;
	struct expression *src_expr, *dest_expr;
	struct statement *store_stmt, *arraycheck, *nullcheck;

	index = stack_pop(ctx->cu->expr_stack);
	arrayref = stack_pop(ctx->cu->expr_stack);

	store_stmt = alloc_statement(STMT_STORE);
	if (!store_stmt)
		goto failed;

	src_expr = array_deref_expr(type, arrayref, index);
	dest_expr = temporary_expr(type, alloc_temporary());
	
	store_stmt->store_src = &src_expr->node;
	store_stmt->store_dest = &dest_expr->node;

	expr_get(dest_expr);
	stack_push(ctx->cu->expr_stack, dest_expr);

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

	bb_add_stmt(ctx->bb, nullcheck);
	bb_add_stmt(ctx->bb, arraycheck);
	bb_add_stmt(ctx->bb, store_stmt);

	return 0;

      failed_nullcheck:
	free_statement(arraycheck);
      failed_arraycheck:
	free_statement(store_stmt);
      failed:
	return -ENOMEM;
}

int convert_iaload(struct parse_context *ctx)
{
	return convert_array_load(ctx, J_INT);
}

int convert_laload(struct parse_context *ctx)
{
	return convert_array_load(ctx, J_LONG);
}

int convert_faload(struct parse_context *ctx)
{
	return convert_array_load(ctx, J_FLOAT);
}

int convert_daload(struct parse_context *ctx)
{
	return convert_array_load(ctx, J_DOUBLE);
}

int convert_aaload(struct parse_context *ctx)
{
	return convert_array_load(ctx, J_REFERENCE);
}

int convert_baload(struct parse_context *ctx)
{
	return convert_array_load(ctx, J_INT);
}

int convert_caload(struct parse_context *ctx)
{
	return convert_array_load(ctx, J_CHAR);
}

int convert_saload(struct parse_context *ctx)
{
	return convert_array_load(ctx, J_SHORT);
}

static int convert_array_store(struct parse_context *ctx, enum vm_type type)
{
	struct expression *value, *index, *arrayref;
	struct statement *store_stmt, *arraycheck, *nullcheck;
	struct expression *src_expr, *dest_expr;

	value = stack_pop(ctx->cu->expr_stack);
	index = stack_pop(ctx->cu->expr_stack);
	arrayref = stack_pop(ctx->cu->expr_stack);

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

	bb_add_stmt(ctx->bb, nullcheck);
	bb_add_stmt(ctx->bb, arraycheck);
	bb_add_stmt(ctx->bb, store_stmt);

	return 0;

      failed_nullcheck:
	free_statement(arraycheck);
      failed_arraycheck:
	free_statement(store_stmt);
      failed:
	return -ENOMEM;
}

int convert_iastore(struct parse_context *ctx)
{
	return convert_array_store(ctx, J_INT);
}

int convert_lastore(struct parse_context *ctx)
{
	return convert_array_store(ctx, J_LONG);
}

int convert_fastore(struct parse_context *ctx)
{
	return convert_array_store(ctx, J_FLOAT);
}

int convert_dastore(struct parse_context *ctx)
{
	return convert_array_store(ctx, J_DOUBLE);
}

int convert_aastore(struct parse_context *ctx)
{
	return convert_array_store(ctx, J_REFERENCE);
}

int convert_bastore(struct parse_context *ctx)
{
	return convert_array_store(ctx, J_INT);
}

int convert_castore(struct parse_context *ctx)
{
	return convert_array_store(ctx, J_CHAR);
}

int convert_sastore(struct parse_context *ctx)
{
	return convert_array_store(ctx, J_SHORT);
}

int convert_new(struct parse_context *ctx)
{
	struct expression *expr;
	unsigned long type_idx;
	struct object *class;

	type_idx = read_u16(ctx->insn_start + 1);
	class = resolveClass(ctx->cu->method->class, type_idx, FALSE);
	if (!class)
		return -EINVAL;

	expr = new_expr(class);
	if (!expr)
		return -ENOMEM;

	stack_push(ctx->cu->expr_stack, expr);

	return 0;
}

int convert_newarray(struct parse_context *ctx)
{
	struct expression *size, *arrayref;
	unsigned long type;

	size = stack_pop(ctx->cu->expr_stack);
	type = read_u8(ctx->insn_start + 1);

	arrayref = newarray_expr(type, size);
	if (!arrayref)
		return -ENOMEM;

	stack_push(ctx->cu->expr_stack, arrayref);

	return 0;
}

int convert_anewarray(struct parse_context *ctx)
{
	struct expression *size,*arrayref;
	unsigned long type_idx;
	struct object *class;

	size = stack_pop(ctx->cu->expr_stack);
	type_idx = read_u16(ctx->insn_start + 1);

	class = resolveClass(ctx->cu->method->class, type_idx, FALSE);

	if (!class)
		return -EINVAL;

	arrayref = anewarray_expr(class,size);
	if (!arrayref)
		return -ENOMEM;

	stack_push(ctx->cu->expr_stack,arrayref);

	return 0;
}

int convert_checkcast(struct parse_context *ctx)
{
	/* TODO */
	return 0;
}
