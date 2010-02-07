/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode load and store
 * instructions to immediate representation of the JIT compiler.
 */

#include "cafebabe/constant_pool.h"

#include "jit/bytecode-to-ir.h"
#include "jit/exception.h"
#include "jit/expression.h"
#include "jit/statement.h"
#include "jit/compiler.h"

#include "vm/bytecode.h"
#include "vm/bytecodes.h"
#include "vm/class.h"
#include "vm/classloader.h"
#include "vm/object.h"
#include "vm/resolve.h"
#include "lib/stack.h"
#include "vm/die.h"

#include <stdint.h>
#include <errno.h>
#include <stdio.h>

static int __convert_const(struct parse_context *ctx,
			   unsigned long long value, enum vm_type vm_type)
{
	struct expression *expr = value_expr(vm_type, value);
	if (!expr)
		return warn("out of memory"), -ENOMEM;

	convert_expression(ctx, expr);
	return 0;
}

int convert_aconst_null(struct parse_context *ctx)
{
	return __convert_const(ctx, 0, J_REFERENCE);
}

int convert_iconst_n(struct parse_context *ctx)
{
	return __convert_const(ctx, ctx->opc - OPC_ICONST_0, J_INT);
}

int convert_lconst_n(struct parse_context *ctx)
{
	return __convert_const(ctx, ctx->opc - OPC_LCONST_0, J_LONG);
}

static int __convert_fconst(struct parse_context *ctx, double value, enum vm_type vm_type)
{
	struct expression *expr = fvalue_expr(vm_type, value);
	if (!expr)
		return warn("out of memory"), -ENOMEM;

	convert_expression(ctx, expr);
	return 0;
}

int convert_fconst_n(struct parse_context *ctx)
{
	return __convert_fconst(ctx, ctx->opc - OPC_FCONST_0, J_FLOAT);
}

int convert_dconst_n(struct parse_context *ctx)
{
	return __convert_fconst(ctx, ctx->opc - OPC_DCONST_0, J_DOUBLE);
}

int convert_bipush(struct parse_context *ctx)
{
	unsigned long long value;

	value = bytecode_read_s8(ctx->buffer);
	return __convert_const(ctx, value, J_INT);
}

int convert_sipush(struct parse_context *ctx)
{
	unsigned long long value;

	value = bytecode_read_s16(ctx->buffer);

	return __convert_const(ctx, value, J_INT);
}

static int __convert_ldc(struct parse_context *ctx, unsigned long cp_idx)
{
	struct vm_class *vmc;
	struct cafebabe_constant_pool *cp;
	struct expression *expr = NULL;

	vmc = ctx->cu->method->class;

	if (cafebabe_class_constant_index_invalid(vmc->class, cp_idx))
		return warn("invalid constant index: %ld", cp_idx), -EINVAL;

	cp = &vmc->class->constant_pool[cp_idx];

	switch (cp->tag) {
	case CAFEBABE_CONSTANT_TAG_INTEGER:
		expr = value_expr(J_INT, cp->integer_.bytes);
		break;
	case CAFEBABE_CONSTANT_TAG_FLOAT:
		expr = fvalue_expr(J_FLOAT, uint32_to_float(cp->float_.bytes));
		break;
	case CAFEBABE_CONSTANT_TAG_STRING: {
		const struct cafebabe_constant_info_utf8 *utf8;
		struct vm_object *string;

		if (cafebabe_class_constant_get_utf8(vmc->class, cp->string.string_index, &utf8))
			return warn("unable to lookup class constant"), -EINVAL;

		string = vm_object_alloc_string_from_utf8(utf8->bytes, utf8->length);
		if (!string)
			return warn("out of memory"), -ENOMEM;

		expr = value_expr(J_REFERENCE, (unsigned long) string);
		break;
	}
	case CAFEBABE_CONSTANT_TAG_LONG:
		expr = value_expr(J_LONG,
			((uint64_t) cp->long_.high_bytes << 32)
			+ (uint64_t) cp->long_.low_bytes);
		break;
	case CAFEBABE_CONSTANT_TAG_DOUBLE:
		expr = fvalue_expr(J_DOUBLE, uint64_to_double(
				cp->double_.low_bytes, cp->double_.high_bytes));
		break;
	case CAFEBABE_CONSTANT_TAG_CLASS: {
		/* Added for JDK 1.5 */
		struct vm_class *ret = vm_class_resolve_class(vmc, cp_idx);

		if (!ret)
			return warn("unable to lookup class"), -EINVAL;

		if (vm_class_ensure_object(ret))
			return -1;

		expr = value_expr(J_REFERENCE, (unsigned long) ret->object);
		break;
	}
	default:
		return warn("unknown tag: %d", cp->tag), -EINVAL;
	}

	if (!expr)
		return warn("out of memory"), -ENOMEM;

	convert_expression(ctx, expr);
	return 0;
}

int convert_ldc(struct parse_context *ctx)
{
	unsigned long idx;

	idx = bytecode_read_u8(ctx->buffer);

	return __convert_ldc(ctx, idx);
}

int convert_ldc_w(struct parse_context *ctx)
{
	unsigned long idx;

	idx = bytecode_read_u16(ctx->buffer);

	return __convert_ldc(ctx, idx);
}

int convert_ldc2_w(struct parse_context *ctx)
{
	unsigned long idx;

	idx = bytecode_read_u16(ctx->buffer);

	return __convert_ldc(ctx, idx);
}

static int convert_load(struct parse_context *ctx, unsigned int index, enum vm_type type)
{
	struct expression *expr;

	expr = local_expr(type, index);
	if (!expr)
		return warn("out of memory"), -ENOMEM;

	convert_expression(ctx, dup_expr(ctx, expr));
	return 0;
}

static unsigned int read_index(struct parse_context *ctx)
{
	if (ctx->is_wide)
		return bytecode_read_u16(ctx->buffer);

	return bytecode_read_u8(ctx->buffer);
}

int convert_iload(struct parse_context *ctx)
{
	unsigned int idx;

	idx = read_index(ctx);

	return convert_load(ctx, idx, J_INT);
}

int convert_lload(struct parse_context *ctx)
{
	unsigned int idx;

	idx = read_index(ctx);

	return convert_load(ctx, idx, J_LONG);
}

int convert_fload(struct parse_context *ctx)
{
	unsigned int idx;

	idx = read_index(ctx);

	return convert_load(ctx, idx, J_FLOAT);
}

int convert_dload(struct parse_context *ctx)
{
	unsigned int idx;

	idx = read_index(ctx);

	return convert_load(ctx, idx, J_DOUBLE);
}

int convert_aload(struct parse_context *ctx)
{
	unsigned int idx;

	idx = read_index(ctx);

	return convert_load(ctx, idx, J_REFERENCE);
}

int convert_iload_n(struct parse_context *ctx)
{
	return convert_load(ctx, ctx->opc - OPC_ILOAD_0, J_INT);
}

int convert_lload_n(struct parse_context *ctx)
{
	return convert_load(ctx, ctx->opc - OPC_LLOAD_0, J_LONG);
}

int convert_fload_n(struct parse_context *ctx)
{
	return convert_load(ctx, ctx->opc - OPC_FLOAD_0, J_FLOAT);
}

int convert_dload_n(struct parse_context *ctx)
{
	return convert_load(ctx, ctx->opc - OPC_DLOAD_0, J_DOUBLE);
}

int convert_aload_n(struct parse_context *ctx)
{
	return convert_load(ctx, ctx->opc - OPC_ALOAD_0, J_REFERENCE);
}

static int convert_store(struct parse_context *ctx, unsigned int index, enum vm_type type)
{
	struct expression *src_expr, *dest_expr;
	struct statement *stmt = alloc_statement(STMT_STORE);
	if (!stmt)
		goto failed;

	dest_expr = local_expr(type, index);
	src_expr = stack_pop(ctx->bb->mimic_stack);

	stmt->store_dest = &dest_expr->node;
	stmt->store_src = &src_expr->node;
	convert_statement(ctx, stmt);
	return 0;
      failed:
	free_statement(stmt);
	return warn("out of memory"), -ENOMEM;
}

int convert_istore(struct parse_context *ctx)
{
	unsigned int idx;

	idx = read_index(ctx);

	return convert_store(ctx, idx, J_INT);
}

int convert_lstore(struct parse_context *ctx)
{
	unsigned int idx;

	idx = read_index(ctx);

	return convert_store(ctx, idx, J_LONG);
}

int convert_fstore(struct parse_context *ctx)
{
	unsigned int idx;

	idx = read_index(ctx);

	return convert_store(ctx, idx, J_FLOAT);
}

int convert_dstore(struct parse_context *ctx)
{
	unsigned int idx;

	idx = read_index(ctx);

	return convert_store(ctx, idx, J_DOUBLE);
}

int convert_astore(struct parse_context *ctx)
{
	unsigned int idx;

	idx = read_index(ctx);

	return convert_store(ctx, idx, J_REFERENCE);
}

int convert_istore_n(struct parse_context *ctx)
{
	return convert_store(ctx, ctx->opc - OPC_ISTORE_0, J_INT);
}

int convert_lstore_n(struct parse_context *ctx)
{
	return convert_store(ctx, ctx->opc - OPC_LSTORE_0, J_LONG);
}

int convert_fstore_n(struct parse_context *ctx)
{
	return convert_store(ctx, ctx->opc - OPC_FSTORE_0, J_FLOAT);
}

int convert_dstore_n(struct parse_context *ctx)
{
	return convert_store(ctx, ctx->opc - OPC_DSTORE_0, J_DOUBLE);
}

int convert_astore_n(struct parse_context *ctx)
{
	return convert_store(ctx, ctx->opc - OPC_ASTORE_0, J_REFERENCE);
}
