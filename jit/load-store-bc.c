/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode load and store
 * instructions to immediate representation of the JIT compiler.
 */

#include <jit/bytecode-converters.h>
#include <jit/compiler.h>
#include <jit/statement.h>

#include <vm/bytecode.h>
#include <vm/bytecodes.h>
#include <vm/byteorder.h>
#include <vm/resolve.h>
#include <vm/stack.h>

#include <errno.h>
#include <stdint.h>

static int __convert_const(struct parse_context *ctx,
			   unsigned long long value, enum vm_type vm_type)
{
	struct expression *expr = value_expr(vm_type, value);
	if (!expr)
		return -ENOMEM;

	stack_push(ctx->cu->mimic_stack, expr);
	return 0;
}

int convert_aconst_null(struct parse_context *ctx)
{
	return __convert_const(ctx, 0, J_REFERENCE);
}

int convert_iconst(struct parse_context *ctx)
{
	return __convert_const(ctx, ctx->opc - OPC_ICONST_0, J_INT);
}

int convert_lconst(struct parse_context *ctx)
{
	return __convert_const(ctx, ctx->opc - OPC_LCONST_0, J_LONG);
}

static int __convert_fconst(struct parse_context *ctx, double value, enum vm_type vm_type)
{
	struct expression *expr = fvalue_expr(vm_type, value);
	if (!expr)
		return -ENOMEM;

	stack_push(ctx->cu->mimic_stack, expr);
	return 0;
}

int convert_fconst(struct parse_context *ctx)
{
	return __convert_fconst(ctx, ctx->opc - OPC_FCONST_0, J_FLOAT);
}

int convert_dconst(struct parse_context *ctx)
{
	return __convert_fconst(ctx, ctx->opc - OPC_DCONST_0, J_DOUBLE);
}

int convert_bipush(struct parse_context *ctx)
{
	unsigned long long value;

	value = read_s8(ctx->insn_start + 1);
	return __convert_const(ctx, value, J_INT);
}

int convert_sipush(struct parse_context *ctx)
{
	unsigned long long value;

	value = read_s16(ctx->insn_start + 1);
	return __convert_const(ctx, value, J_INT);
}

static int __convert_ldc(struct parse_context *ctx, unsigned long cp_idx)
{
	struct constant_pool *cp;
	struct expression *expr;
	struct classblock *cb;

	cb = CLASS_CB(ctx->cu->method->class);
	cp = &cb->constant_pool;

	uint8_t type = CP_TYPE(cp, cp_idx);
	switch (type) {
	case CONSTANT_Integer:
		expr = value_expr(J_INT, CP_INTEGER(cp, cp_idx));
		break;
	case CONSTANT_Float:
		expr = fvalue_expr(J_FLOAT, CP_FLOAT(cp, cp_idx));
		break;
	case CONSTANT_String: {
		struct object *string = resolve_string(cp, CP_STRING(cp, cp_idx));

		expr = value_expr(J_REFERENCE, (unsigned long) string);
		break;
	}
	case CONSTANT_Long:
		expr = value_expr(J_LONG, CP_LONG(cp, cp_idx));
		break;
	case CONSTANT_Double:
		expr = fvalue_expr(J_DOUBLE, CP_DOUBLE(cp, cp_idx));
		break;
	default:
		return -EINVAL;
	}

	if (!expr)
		return -ENOMEM;

	stack_push(ctx->cu->mimic_stack, expr);
	return 0;
}

int convert_ldc(struct parse_context *ctx)
{
	unsigned long idx;

	idx = read_u8(ctx->insn_start + 1);
	return __convert_ldc(ctx, idx);
}

int convert_ldc_w(struct parse_context *ctx)
{
	unsigned long idx;

	idx = read_u16(ctx->insn_start + 1);
	return __convert_ldc(ctx, idx);
}

int convert_ldc2_w(struct parse_context *ctx)
{
	unsigned long idx;

	idx = read_u16(ctx->insn_start + 1);
	return __convert_ldc(ctx, idx);
}

static int convert_load(struct parse_context *ctx, unsigned char index, enum vm_type type)
{
	struct expression *expr;

	expr = local_expr(type, index);
	if (!expr)
		return -ENOMEM;

	stack_push(ctx->cu->mimic_stack, expr);
	return 0;
}

int convert_iload(struct parse_context *ctx)
{
	unsigned char idx;

	idx = read_u8(ctx->insn_start + 1);
	return convert_load(ctx, idx, J_INT);
}

int convert_lload(struct parse_context *ctx)
{
	unsigned char idx;

	idx = read_u8(ctx->insn_start + 1);
	return convert_load(ctx, idx, J_LONG);
}

int convert_fload(struct parse_context *ctx)
{
	unsigned char idx;

	idx = read_u8(ctx->insn_start + 1);
	return convert_load(ctx, idx, J_FLOAT);
}

int convert_dload(struct parse_context *ctx)
{
	unsigned char idx;

	idx = read_u8(ctx->insn_start + 1);
	return convert_load(ctx, idx, J_DOUBLE);
}

int convert_aload(struct parse_context *ctx)
{
	unsigned char idx;

	idx = read_u8(ctx->insn_start + 1);
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

static int convert_store(struct parse_context *ctx, unsigned long index, enum vm_type type)
{
	struct expression *src_expr, *dest_expr;
	struct statement *stmt = alloc_statement(STMT_STORE);
	if (!stmt)
		goto failed;

	dest_expr = local_expr(type, index);
	src_expr = stack_pop(ctx->cu->mimic_stack);

	stmt->store_dest = &dest_expr->node;
	stmt->store_src = &src_expr->node;
	bb_add_stmt(ctx->bb, stmt);
	return 0;
      failed:
	free_statement(stmt);
	return -ENOMEM;
}

int convert_istore(struct parse_context *ctx)
{
	unsigned char idx;

	idx = read_u8(ctx->insn_start + 1);
	return convert_store(ctx, idx, J_INT);
}

int convert_lstore(struct parse_context *ctx)
{
	unsigned char idx;

	idx = read_u8(ctx->insn_start + 1);
	return convert_store(ctx, idx, J_LONG);
}

int convert_fstore(struct parse_context *ctx)
{
	unsigned char idx;

	idx = read_u8(ctx->insn_start + 1);
	return convert_store(ctx, idx, J_FLOAT);
}

int convert_dstore(struct parse_context *ctx)
{
	unsigned char idx;

	idx = read_u8(ctx->insn_start + 1);
	return convert_store(ctx, idx, J_DOUBLE);
}

int convert_astore(struct parse_context *ctx)
{
	unsigned char idx;

	idx = read_u8(ctx->insn_start + 1);
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
