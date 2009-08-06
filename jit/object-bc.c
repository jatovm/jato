/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode arithmetic
 * instructions to immediate representation of the JIT compiler.
 */

#include "jit/bytecode-to-ir.h"
#include "jit/statement.h"
#include "jit/compiler.h"
#include "jit/args.h"

#include "vm/bytecode.h"
#include "vm/bytecodes.h"
#include "vm/classloader.h"
#include "vm/field.h"
#include "vm/object.h"
#include "vm/stack.h"
#include "vm/die.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

static char *class_name_to_array_name(const char *class_name)
{
	char *array_name = malloc(strlen(class_name) + 4);

	if (class_name[0] == '[') {
		strcpy(array_name, "[");
		strcat(array_name, class_name);
	} else {
		strcpy(array_name, "[L");
		strcat(array_name, class_name);
		strcat(array_name, ";");
	}

	return array_name;
}

static struct vm_class *class_to_array_class(struct vm_class *class)
{
	struct vm_class *array_class;
	char *array_class_name;

	/* XXX: This is not entirely right. We need to make sure that we're
	 * using the same class loader as the original class. We don't support
	 * multiple (different) class loaders yet. */
	array_class_name = class_name_to_array_name(class->name);
	array_class = classloader_load(array_class_name);
	free(array_class_name);

	return array_class;
}

static struct vm_field *lookup_field(struct parse_context *ctx)
{
	unsigned short index;

	index = bytecode_read_u16(ctx->buffer);

	return vm_class_resolve_field_recursive(ctx->cu->method->class, index);
}

int convert_getstatic(struct parse_context *ctx)
{
	struct expression *value_dup;
	struct expression *value;
	struct vm_field *fb;

	fb = lookup_field(ctx);
	if (!fb)
		return warn("field lookup failed"), -EINVAL;

	value = class_field_expr(vm_field_type(fb), fb);
	if (!value)
		return warn("out of memory"), -ENOMEM;

	value_dup = dup_expr(ctx, value);
	if (!value_dup)
		return warn("out of memory"), -ENOMEM;

	convert_expression(ctx, value_dup);
	return 0;
}

int convert_putstatic(struct parse_context *ctx)
{
	struct vm_field *fb;
	struct statement *store_stmt;
	struct expression *dest, *src;

	fb = lookup_field(ctx);
	if (!fb)
		return warn("field lookup failed"), -EINVAL;

	src = stack_pop(ctx->bb->mimic_stack);
	dest = class_field_expr(vm_field_type(fb), fb);
	if (!dest)
		return warn("out of memory"), -ENOMEM;

	store_stmt = alloc_statement(STMT_STORE);
	if (!store_stmt) {
		expr_put(dest);
		return warn("out of memory"), -ENOMEM;
	}
	store_stmt->store_dest = &dest->node;
	store_stmt->store_src = &src->node;
	convert_statement(ctx, store_stmt);

	return 0;
}

int convert_getfield(struct parse_context *ctx)
{
	struct expression *objectref;
	struct expression *value_dup;
	struct expression *value;
	struct vm_field *fb;

	fb = lookup_field(ctx);
	if (!fb)
		return warn("field lookup failed"), -EINVAL;

	objectref = null_check_expr(stack_pop(ctx->bb->mimic_stack));

	value = instance_field_expr(vm_field_type(fb), fb, objectref);
	if (!value)
		return warn("out of memory"), -ENOMEM;

	value_dup = dup_expr(ctx, value);
	if (!value_dup)
		return warn("out of memory"), -ENOMEM;

	convert_expression(ctx, value_dup);
	return 0;
}

int convert_putfield(struct parse_context *ctx)
{
	struct expression *dest, *src;
	struct statement *store_stmt;
	struct expression *objectref;
	struct vm_field *fb;

	fb = lookup_field(ctx);
	if (!fb)
		return warn("field lookup failed"), -EINVAL;

	src = stack_pop(ctx->bb->mimic_stack);
	objectref = null_check_expr(stack_pop(ctx->bb->mimic_stack));
	dest = instance_field_expr(vm_field_type(fb), fb, objectref);
	if (!dest)
		return warn("out of memory"), -ENOMEM;

	store_stmt = alloc_statement(STMT_STORE);
	if (!store_stmt) {
		expr_put(dest);
		return warn("out of memory"), -ENOMEM;
	}
	store_stmt->store_dest = &dest->node;
	store_stmt->store_src = &src->node;
	convert_statement(ctx, store_stmt);

	return 0;
}

static int convert_array_load(struct parse_context *ctx, enum vm_type type)
{
	struct expression *index, *arrayref;
	struct expression *src_expr, *dest_expr;
	struct statement *store_stmt, *arraycheck;
	struct var_info *temporary;
	struct expression *arrayref_pure;
	struct expression *index_pure;

	index = stack_pop(ctx->bb->mimic_stack);
	arrayref = null_check_expr(stack_pop(ctx->bb->mimic_stack));
	if (!arrayref)
		goto failed;

	store_stmt = alloc_statement(STMT_STORE);
	if (!store_stmt)
		goto failed;

	/*
	 * We have to assure that arrayref and index expressions
	 * passed to array_deref_expr() do not have side effects
	 * because src_expr is connected to both STMT_STORE and
	 * STMT_ARRAY_CHECK.
	 */
	arrayref_pure = get_pure_expr(ctx, arrayref);
	index_pure = get_pure_expr(ctx, index);
	src_expr = array_deref_expr(type, arrayref_pure, index_pure);

	temporary = get_var(ctx->cu, type);
	dest_expr = temporary_expr(type, NULL, temporary);

	store_stmt->store_src = &src_expr->node;
	store_stmt->store_dest = &dest_expr->node;

	expr_get(dest_expr);
	convert_expression(ctx, dest_expr);

	arraycheck = alloc_statement(STMT_ARRAY_CHECK);
	if (!arraycheck)
		goto failed_arraycheck;

	expr_get(src_expr);
	arraycheck->expression = &src_expr->node;

	convert_statement(ctx, arraycheck);
	convert_statement(ctx, store_stmt);

	return 0;

      failed_arraycheck:
	free_statement(store_stmt);
      failed:
	return warn("out of memory"), -ENOMEM;
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
	return convert_array_load(ctx, J_BYTE);
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
	struct statement *store_stmt, *arraycheck;
	struct statement *array_store_check_stmt;
	struct expression *src_expr, *dest_expr;
	struct expression *arrayref_pure;
	struct expression *index_pure;

	value = stack_pop(ctx->bb->mimic_stack);
	index = stack_pop(ctx->bb->mimic_stack);

	arrayref = null_check_expr(stack_pop(ctx->bb->mimic_stack));
	if (!arrayref)
		goto failed;

	store_stmt = alloc_statement(STMT_STORE);
	if (!store_stmt)
		goto failed;


	arrayref_pure = get_pure_expr(ctx, arrayref);
	index_pure = get_pure_expr(ctx, index);
	dest_expr = array_deref_expr(type, arrayref_pure, index_pure);

	src_expr = dup_expr(ctx, value);

	store_stmt->store_dest = &dest_expr->node;
	store_stmt->store_src = &src_expr->node;

	arraycheck = alloc_statement(STMT_ARRAY_CHECK);
	if (!arraycheck)
		goto failed_arraycheck;

	expr_get(dest_expr);
	arraycheck->expression = &dest_expr->node;

	array_store_check_stmt = alloc_statement(STMT_ARRAY_STORE_CHECK);
	if (!array_store_check_stmt)
		goto failed_array_store_check_stmt;

	expr_get(src_expr);
	array_store_check_stmt->store_check_src = &src_expr->node;
	expr_get(arrayref_pure);
	array_store_check_stmt->store_check_array = &arrayref_pure->node;

	convert_statement(ctx, arraycheck);
	convert_statement(ctx, array_store_check_stmt);
	convert_statement(ctx, store_stmt);

	return 0;

      failed_array_store_check_stmt:
	free_statement(arraycheck);
      failed_arraycheck:
	free_statement(store_stmt);
      failed:
	return warn("out of memory"), -ENOMEM;
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
	return convert_array_store(ctx, J_BYTE);
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
	struct vm_class *class;

	type_idx = bytecode_read_u16(ctx->buffer);
	class = vm_class_resolve_class(ctx->cu->method->class, type_idx);
	if (!class)
		return warn("unable to resolve class"), -EINVAL;

	expr = new_expr(class);
	if (!expr)
		return warn("out of memory"), -ENOMEM;

	convert_expression(ctx, expr);

	return 0;
}

int convert_newarray(struct parse_context *ctx)
{
	struct expression *size, *arrayref;
	struct expression *size_check;
	unsigned long type;

	size = stack_pop(ctx->bb->mimic_stack);
	type = bytecode_read_u8(ctx->buffer);

	size_check = array_size_check_expr(size);
	if (!size_check)
		return warn("out of memory"), -ENOMEM;

	arrayref = newarray_expr(type, size_check);
	if (!arrayref)
		return warn("out of memory"), -ENOMEM;

	convert_expression(ctx, arrayref);

	return 0;
}

int convert_anewarray(struct parse_context *ctx)
{
	struct expression *size_check;
	struct expression *size,*arrayref;
	unsigned long type_idx;
	struct vm_class *class, *array_class;

	size = stack_pop(ctx->bb->mimic_stack);
	type_idx = bytecode_read_u16(ctx->buffer);

	class = vm_class_resolve_class(ctx->cu->method->class, type_idx);
	if (!class)
		return warn("unable to resolve class"), -EINVAL;

	array_class = class_to_array_class(class);
	if (!array_class)
		return warn("conversion failed"), -EINVAL;

	size_check = array_size_check_expr(size);
	if (!size_check)
		return warn("out of memory"), -ENOMEM;

	arrayref = anewarray_expr(array_class, size_check);
	if (!arrayref)
		return warn("out of memory"), -ENOMEM;

	convert_expression(ctx, arrayref);

	return 0;
}

int convert_multianewarray(struct parse_context *ctx)
{
	struct expression *size_check;
	struct expression *arrayref;
	struct expression *args_list;
	unsigned long type_idx;
	unsigned char dimension;
	struct vm_class *class;
	struct vm_method *method;

	type_idx = bytecode_read_u16(ctx->buffer);
	dimension = bytecode_read_u8(ctx->buffer);
	class = vm_class_resolve_class(ctx->cu->method->class, type_idx);
	method = ctx->cu->method;
	if (!class)
		return warn("out of memory"), -ENOMEM;

	arrayref = multianewarray_expr(class);
	if (!arrayref)
		return warn("out of memory"), -ENOMEM;

	args_list = convert_args(ctx->bb->mimic_stack, dimension, method);

	size_check = multiarray_size_check_expr(args_list);
	if (!size_check)
		return warn("out of memory"), -ENOMEM;

	arrayref->multianewarray_dimensions = &size_check->node;

	convert_expression(ctx, arrayref);

	return 0;
}

int convert_arraylength(struct parse_context *ctx)
{
	struct expression *arrayref, *arraylength_exp;

	arrayref = null_check_expr(stack_pop(ctx->bb->mimic_stack));

	arraylength_exp = arraylength_expr(arrayref);
	if (!arraylength_exp)
		return warn("out of memory"), -ENOMEM;

	convert_expression(ctx, arraylength_exp);

	return 0;
}

int convert_instanceof(struct parse_context *ctx)
{
	struct expression *objectref, *expr;
	struct vm_class *class;
	unsigned long type_idx;

	objectref = stack_pop(ctx->bb->mimic_stack);

	type_idx = bytecode_read_u16(ctx->buffer);
	class = vm_class_resolve_class(ctx->cu->method->class, type_idx);
	if (!class)
		return warn("unable to resolve class"), -EINVAL;

	expr = instanceof_expr(objectref, class);
	if (!expr)
		return warn("out of memory"), -ENOMEM;

	convert_expression(ctx, expr);

	return 0;
}

int convert_checkcast(struct parse_context *ctx)
{
	struct expression *object_ref_tmp;
	struct vm_class *class;
	struct statement *checkcast_stmt;
	unsigned long type_idx;

	object_ref_tmp = dup_expr(ctx, stack_pop(ctx->bb->mimic_stack));

	type_idx = bytecode_read_u16(ctx->buffer);
	class = vm_class_resolve_class(ctx->cu->method->class, type_idx);
	if (!class)
		return warn("out of memory"), -ENOMEM;

	checkcast_stmt = alloc_statement(STMT_CHECKCAST);
	if (!checkcast_stmt)
		return warn("out of memory"), -ENOMEM;

	checkcast_stmt->checkcast_class = class;
	checkcast_stmt->checkcast_ref = &object_ref_tmp->node;

	convert_statement(ctx, checkcast_stmt);
	convert_expression(ctx, expr_get(object_ref_tmp));

	return 0;
}

int convert_monitorenter(struct parse_context *ctx)
{
	struct expression *nullcheck;
	struct expression *exp;
	struct statement *stmt;

	exp = stack_pop(ctx->bb->mimic_stack);

	nullcheck = null_check_expr(exp);
	if (!nullcheck)
		return warn("out of memory"), -ENOMEM;

	stmt = alloc_statement(STMT_MONITOR_ENTER);
	if (!stmt)
		return warn("out of memory"), -ENOMEM;

	expr_get(exp);
	stmt->expression = &nullcheck->node;
	convert_statement(ctx, stmt);

	return 0;
}

int convert_monitorexit(struct parse_context *ctx)
{
	struct expression *nullcheck;
	struct expression *exp;
	struct statement *stmt;

	exp = stack_pop(ctx->bb->mimic_stack);

	nullcheck = null_check_expr(exp);
	if (!nullcheck)
		return warn("out of memory"), -ENOMEM;

	stmt = alloc_statement(STMT_MONITOR_EXIT);
	if (!stmt)
		return warn("out of memory"), -ENOMEM;

	expr_get(exp);
	stmt->expression = &nullcheck->node;
	convert_statement(ctx, stmt);

	return 0;
}
