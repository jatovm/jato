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
#include <jit/jit-compiler.h>
#include <jit/statement.h>

#include <vm/bytecode.h>
#include <vm/bytecodes.h>
#include <vm/byteorder.h>
#include <vm/stack.h>

#include <errno.h>

static int __convert_const(enum vm_type vm_type, unsigned long long value,
			   struct stack *expr_stack)
{
	struct expression *expr = value_expr(vm_type, value);
	if (!expr)
		return -ENOMEM;

	stack_push(expr_stack, expr);
	return 0;
}

int convert_aconst_null(struct parse_context *ctx)
{
	return __convert_const(J_REFERENCE, 0, ctx->cu->expr_stack);
}

int convert_iconst(struct parse_context *ctx)
{
	return __convert_const(J_INT, read_u8(ctx->code, ctx->offset) - OPC_ICONST_0,
			       ctx->cu->expr_stack);
}

int convert_lconst(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return __convert_const(J_LONG, read_u8(code, ctx->offset) - OPC_LCONST_0,
			       ctx->cu->expr_stack);
}

static int __convert_fconst(enum vm_type vm_type,
			    double value, struct stack *expr_stack)
{
	struct expression *expr = fvalue_expr(vm_type, value);
	if (!expr)
		return -ENOMEM;

	stack_push(expr_stack, expr);
	return 0;
}

int convert_fconst(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return __convert_fconst(J_FLOAT,
				read_u8(code, ctx->offset) - OPC_FCONST_0,
				ctx->cu->expr_stack);
}

int convert_dconst(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return __convert_fconst(J_DOUBLE,
				read_u8(code, ctx->offset) - OPC_DCONST_0,
				ctx->cu->expr_stack);
}

int convert_bipush(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return __convert_const(J_INT, read_s8(code, ctx->offset + 1),
			       ctx->cu->expr_stack);
}

int convert_sipush(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return __convert_const(J_INT,
			       read_s16(code, ctx->offset + 1),
			       ctx->cu->expr_stack);
}

static int __convert_ldc(struct constant_pool *cp,
			 unsigned long cp_idx, struct stack *expr_stack)
{
	struct expression *expr;

	u1 type = CP_TYPE(cp, cp_idx);
	switch (type) {
	case CONSTANT_Integer:
		expr = value_expr(J_INT, CP_INTEGER(cp, cp_idx));
		break;
	case CONSTANT_Float:
		expr = fvalue_expr(J_FLOAT, CP_FLOAT(cp, cp_idx));
		break;
	case CONSTANT_String:
		expr = value_expr(J_REFERENCE, CP_STRING(cp, cp_idx));
		break;
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

	stack_push(expr_stack, expr);
	return 0;
}

int convert_ldc(struct parse_context *ctx)
{
	struct classblock *cb = CLASS_CB(ctx->cu->method->class);
	unsigned char *code = ctx->code;
	return __convert_ldc(&cb->constant_pool,
			     read_u8(code, ctx->offset + 1), ctx->cu->expr_stack);
}

int convert_ldc_w(struct parse_context *ctx)
{
	struct classblock *cb = CLASS_CB(ctx->cu->method->class);
	unsigned char *code = ctx->code;
	return __convert_ldc(&cb->constant_pool,
			     read_u16(code, ctx->offset + 1),
			     ctx->cu->expr_stack);
}

int convert_ldc2_w(struct parse_context *ctx)
{
	struct classblock *cb = CLASS_CB(ctx->cu->method->class);
	unsigned char *code = ctx->code;
	return __convert_ldc(&cb->constant_pool,
			     read_u16(code, ctx->offset + 1),
			     ctx->cu->expr_stack);
}

static int convert_load(struct parse_context *ctx, unsigned char index,
			enum vm_type type)
{
	struct expression *expr;

	expr = local_expr(type, index);
	if (!expr)
		return -ENOMEM;

	stack_push(ctx->cu->expr_stack, expr);
	return 0;
}

int convert_iload(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return convert_load(ctx, read_u8(code, ctx->offset + 1), J_INT);
}

int convert_lload(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return convert_load(ctx, read_u8(code, ctx->offset + 1), J_LONG);
}

int convert_fload(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return convert_load(ctx, read_u8(code, ctx->offset + 1), J_FLOAT);
}

int convert_dload(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return convert_load(ctx, read_u8(code, ctx->offset + 1), J_DOUBLE);
}

int convert_aload(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return convert_load(ctx, read_u8(code, ctx->offset + 1), J_REFERENCE);
}

int convert_iload_n(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return convert_load(ctx, read_u8(code, ctx->offset) - OPC_ILOAD_0, J_INT);
}

int convert_lload_n(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return convert_load(ctx, read_u8(code, ctx->offset) - OPC_LLOAD_0, J_LONG);
}

int convert_fload_n(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return convert_load(ctx, read_u8(code, ctx->offset) - OPC_FLOAD_0, J_FLOAT);
}

int convert_dload_n(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return convert_load(ctx, read_u8(code, ctx->offset) - OPC_DLOAD_0, J_DOUBLE);
}

int convert_aload_n(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return convert_load(ctx, read_u8(code, ctx->offset) - OPC_ALOAD_0,
			    J_REFERENCE);
}

static int convert_store(struct parse_context *ctx, enum vm_type type, unsigned long index)
{
	struct expression *src_expr, *dest_expr;
	struct statement *stmt = alloc_statement(STMT_STORE);
	if (!stmt)
		goto failed;

	dest_expr = local_expr(type, index);
	src_expr = stack_pop(ctx->cu->expr_stack);

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
	unsigned char *code = ctx->code;
	return convert_store(ctx, J_INT, read_u8(code, ctx->offset + 1));
}

int convert_lstore(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return convert_store(ctx, J_LONG, read_u8(code, ctx->offset + 1));
}

int convert_fstore(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return convert_store(ctx, J_FLOAT, read_u8(code, ctx->offset + 1));
}

int convert_dstore(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return convert_store(ctx, J_DOUBLE, read_u8(code, ctx->offset + 1));
}

int convert_astore(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return convert_store(ctx, J_REFERENCE, read_u8(code, ctx->offset + 1));
}

int convert_istore_n(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return convert_store(ctx, J_INT, read_u8(code, ctx->offset) - OPC_ISTORE_0);
}

int convert_lstore_n(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return convert_store(ctx, J_LONG, read_u8(code, ctx->offset) - OPC_LSTORE_0);
}

int convert_fstore_n(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return convert_store(ctx, J_FLOAT, read_u8(code, ctx->offset) - OPC_FSTORE_0);
}

int convert_dstore_n(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return convert_store(ctx, J_DOUBLE, read_u8(code, ctx->offset) - OPC_DSTORE_0);
}

int convert_astore_n(struct parse_context *ctx)
{
	unsigned char *code = ctx->code;
	return convert_store(ctx, J_REFERENCE,
			     read_u8(code, ctx->offset) - OPC_ASTORE_0);
}
