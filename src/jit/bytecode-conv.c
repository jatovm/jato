/*
 * Copyright (C) 2005  Pekka Enberg
 *
 * This file contains functions for converting Java bytecode to three-address
 * code.
 */

#include <statement.h>
#include <byteorder.h>
#include <operand-stack.h>

#include <stdlib.h>
#include <assert.h>

struct conversion_context {
	struct classblock *cb;
	unsigned char *code;
	unsigned long len;
	struct operand_stack *stack;
};

static struct statement *convert_nop(struct conversion_context *context)
{
	return alloc_stmt(STMT_NOP);
}

static struct statement *convert_aconst_null(struct conversion_context *context)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		operand_set_const(stmt->s_left, CONST_REFERENCE, 0);
		stack_push(context->stack, stmt->s_target->temporary);
	}
	return stmt;
}

static struct statement *convert_iconst(struct conversion_context *context)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		operand_set_const(stmt->s_left, CONST_INT,
				  context->code[0] - OPC_ICONST_0);
		stack_push(context->stack, stmt->s_target->temporary);
	}
	return stmt;
}

static struct statement *convert_lconst(struct conversion_context *context)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		operand_set_const(stmt->s_left, CONST_LONG,
				  context->code[0] - OPC_LCONST_0);
		stack_push(context->stack, stmt->s_target->temporary);
	}
	return stmt;
}

static struct statement *convert_fconst(struct conversion_context *context)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		operand_set_fconst(stmt->s_left, CONST_FLOAT,
				   context->code[0] - OPC_FCONST_0);
		stack_push(context->stack, stmt->s_target->temporary);
	}
	return stmt;
}

static struct statement *convert_dconst(struct conversion_context *context)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		operand_set_fconst(stmt->s_left, CONST_DOUBLE,
				   context->code[0] - OPC_DCONST_0);
		stack_push(context->stack, stmt->s_target->temporary);
	}
	return stmt;
}

static struct statement *convert_bipush(struct conversion_context *context)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		operand_set_const(stmt->s_left, CONST_INT, (char)context->code[1]);
		stack_push(context->stack, stmt->s_target->temporary);
	}
	return stmt;
}

static struct statement *convert_sipush(struct conversion_context *context)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		operand_set_const(stmt->s_left, CONST_INT,
				  (short)be16_to_cpu(*(u2 *) & context->code[1]));
		stack_push(context->stack, stmt->s_target->temporary);
	}
	return stmt;
}

static struct statement *__convert_ldc(struct constant_pool *cp,
				       unsigned long cp_idx,
				       struct operand_stack *stack)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		u1 type = CP_TYPE(cp, cp_idx);
		ConstantPoolEntry entry = be64_to_cpu(CP_INFO(cp, cp_idx));
		switch (type) {
		case CONSTANT_Integer:
			operand_set_const(stmt->s_left, CONST_INT, entry);
			break;
		case CONSTANT_Float:
			operand_set_fconst(stmt->s_left, CONST_FLOAT,
					   *(float *)&entry);
			break;
		case CONSTANT_String:
			operand_set_const(stmt->s_left, CONST_REFERENCE,
					  entry);
			break;
		case CONSTANT_Long:
			operand_set_const(stmt->s_left, CONST_LONG, entry);
			break;
		case CONSTANT_Double:
			operand_set_fconst(stmt->s_left, CONST_DOUBLE,
					   *(double *)&entry);
			stmt->s_left->constant.type = CONST_DOUBLE;
			break;
		default:
			assert(!"unknown constant type");
		}
		stack_push(stack, stmt->s_target->temporary);
	}
	return stmt;
}

static struct statement *convert_ldc(struct conversion_context *context)
{
	return __convert_ldc(&context->cb->constant_pool, context->code[1],
			     context->stack);
}

static struct statement *convert_ldc_w(struct conversion_context *context)
{
	return __convert_ldc(&context->cb->constant_pool,
			     be16_to_cpu(*(u2 *) & context->code[1]),
			     context->stack);
}

static struct statement *convert_ldc2_w(struct conversion_context *context)
{
	return __convert_ldc(&context->cb->constant_pool,
			     be16_to_cpu(*(u2 *) & context->code[1]),
			     context->stack);
}

static struct statement *__convert_load(unsigned char index,
					enum jvm_type type,
					struct operand_stack *stack)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		operand_set_local_var(stmt->s_left, type, index);
		stack_push(stack, stmt->s_target->temporary);
	}
	return stmt;
}

static struct statement *convert_load(struct conversion_context *context,
				      enum jvm_type type)
{
	return __convert_load(context->code[1], type, context->stack);
}

static struct statement *convert_iload(struct conversion_context *context)
{
	return convert_load(context, J_INT);
}

static struct statement *convert_lload(struct conversion_context *context)
{
	return convert_load(context, J_LONG);
}

static struct statement *convert_fload(struct conversion_context *context)
{
	return convert_load(context, J_FLOAT);
}

static struct statement *convert_dload(struct conversion_context *context)
{
	return convert_load(context, J_DOUBLE);
}

static struct statement *convert_aload(struct conversion_context *context)
{
	return convert_load(context, J_REFERENCE);
}

static struct statement *convert_iload_x(struct conversion_context *context)
{
	return __convert_load(context->code[0] - OPC_ILOAD_0, J_INT,
			      context->stack);
}

static struct statement *convert_lload_x(struct conversion_context *context)
{
	return __convert_load(context->code[0] - OPC_LLOAD_0, J_LONG,
			      context->stack);
}

static struct statement *convert_fload_x(struct conversion_context *context)
{
	return __convert_load(context->code[0] - OPC_FLOAD_0, J_FLOAT,
			      context->stack);
}

static struct statement *convert_dload_x(struct conversion_context *context)
{
	return __convert_load(context->code[0] - OPC_DLOAD_0, J_DOUBLE,
			      context->stack);
}

static struct statement *convert_aload_x(struct conversion_context *context)
{
	return __convert_load(context->code[0] - OPC_ALOAD_0, J_REFERENCE,
			      context->stack);
}

static struct statement *convert_xaload(struct conversion_context *context)
{
	unsigned long index = stack_pop(context->stack);
	unsigned long arrayref = stack_pop(context->stack);
	struct statement *assign = alloc_stmt(STMT_ASSIGN);

	assert(assign);
	operand_set_arrayref(assign->s_left, arrayref, index);

	stack_push(context->stack, assign->s_target->temporary);

	struct statement *arraycheck = alloc_stmt(STMT_ARRAY_CHECK);
	assert(arraycheck);
	operand_set_temporary(arraycheck->s_left, arrayref);
	operand_set_temporary(arraycheck->s_right, index);
	arraycheck->s_next = assign;

	struct statement *nullcheck = alloc_stmt(STMT_NULL_CHECK);
	if (nullcheck) {
		operand_set_temporary(nullcheck->s_left, arrayref);
		nullcheck->s_next = arraycheck;
	}
	return nullcheck;
}

static struct statement *convert_store(enum jvm_type type,
				       unsigned long index,
				       struct operand_stack *stack)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		operand_set_local_var(stmt->s_target, type, index);
		stmt->s_left->type = OPERAND_TEMPORARY;
		stmt->s_left->temporary = stack_pop(stack);
	}
	return stmt;
}

static struct statement *convert_istore(struct conversion_context *context)
{
	return convert_store(J_INT, context->code[1], context->stack);
}

static struct statement *convert_lstore(struct conversion_context *context)
{
	return convert_store(J_LONG, context->code[1], context->stack);
}

static struct statement *convert_fstore(struct conversion_context *context)
{
	return convert_store(J_FLOAT, context->code[1], context->stack);
}

static struct statement *convert_dstore(struct conversion_context *context)
{
	return convert_store(J_DOUBLE, context->code[1], context->stack);
}

static struct statement *convert_astore(struct conversion_context *context)
{
	return convert_store(J_REFERENCE, context->code[1], context->stack);
}

typedef struct statement *(*convert_fn_t) (struct conversion_context *);

struct converter {
	convert_fn_t convert;
	unsigned long require;
};

#define DECLARE_CONVERTER(opc, fn, bytes) \
		[opc] = { .convert = fn, .require = bytes }

static struct converter converters[] = {
	DECLARE_CONVERTER(OPC_NOP, convert_nop, 1),
	DECLARE_CONVERTER(OPC_ACONST_NULL, convert_aconst_null, 1),
	DECLARE_CONVERTER(OPC_ICONST_M1, convert_iconst, 1),
	DECLARE_CONVERTER(OPC_ICONST_0, convert_iconst, 1),
	DECLARE_CONVERTER(OPC_ICONST_1, convert_iconst, 1),
	DECLARE_CONVERTER(OPC_ICONST_2, convert_iconst, 1),
	DECLARE_CONVERTER(OPC_ICONST_3, convert_iconst, 1),
	DECLARE_CONVERTER(OPC_ICONST_4, convert_iconst, 1),
	DECLARE_CONVERTER(OPC_ICONST_5, convert_iconst, 1),
	DECLARE_CONVERTER(OPC_LCONST_0, convert_lconst, 1),
	DECLARE_CONVERTER(OPC_LCONST_1, convert_lconst, 1),
	DECLARE_CONVERTER(OPC_FCONST_0, convert_fconst, 1),
	DECLARE_CONVERTER(OPC_FCONST_1, convert_fconst, 1),
	DECLARE_CONVERTER(OPC_FCONST_2, convert_fconst, 1),
	DECLARE_CONVERTER(OPC_DCONST_0, convert_dconst, 1),
	DECLARE_CONVERTER(OPC_DCONST_1, convert_dconst, 1),
	DECLARE_CONVERTER(OPC_BIPUSH, convert_bipush, 2),
	DECLARE_CONVERTER(OPC_SIPUSH, convert_sipush, 2),
	DECLARE_CONVERTER(OPC_LDC, convert_ldc, 2),
	DECLARE_CONVERTER(OPC_LDC_W, convert_ldc_w, 3),
	DECLARE_CONVERTER(OPC_LDC2_W, convert_ldc2_w, 3),
	DECLARE_CONVERTER(OPC_ILOAD, convert_iload, 2),
	DECLARE_CONVERTER(OPC_LLOAD, convert_lload, 2),
	DECLARE_CONVERTER(OPC_FLOAD, convert_fload, 2),
	DECLARE_CONVERTER(OPC_DLOAD, convert_dload, 2),
	DECLARE_CONVERTER(OPC_ALOAD, convert_aload, 2),
	DECLARE_CONVERTER(OPC_ILOAD_0, convert_iload_x, 2),
	DECLARE_CONVERTER(OPC_ILOAD_1, convert_iload_x, 2),
	DECLARE_CONVERTER(OPC_ILOAD_2, convert_iload_x, 2),
	DECLARE_CONVERTER(OPC_ILOAD_3, convert_iload_x, 2),
	DECLARE_CONVERTER(OPC_LLOAD_0, convert_lload_x, 2),
	DECLARE_CONVERTER(OPC_LLOAD_1, convert_lload_x, 2),
	DECLARE_CONVERTER(OPC_LLOAD_2, convert_lload_x, 2),
	DECLARE_CONVERTER(OPC_LLOAD_3, convert_lload_x, 2),
	DECLARE_CONVERTER(OPC_FLOAD_0, convert_fload_x, 2),
	DECLARE_CONVERTER(OPC_FLOAD_1, convert_fload_x, 2),
	DECLARE_CONVERTER(OPC_FLOAD_2, convert_fload_x, 2),
	DECLARE_CONVERTER(OPC_FLOAD_3, convert_fload_x, 2),
	DECLARE_CONVERTER(OPC_DLOAD_0, convert_dload_x, 2),
	DECLARE_CONVERTER(OPC_DLOAD_1, convert_dload_x, 2),
	DECLARE_CONVERTER(OPC_DLOAD_2, convert_dload_x, 2),
	DECLARE_CONVERTER(OPC_DLOAD_3, convert_dload_x, 2),
	DECLARE_CONVERTER(OPC_ALOAD_0, convert_aload_x, 2),
	DECLARE_CONVERTER(OPC_ALOAD_1, convert_aload_x, 2),
	DECLARE_CONVERTER(OPC_ALOAD_2, convert_aload_x, 2),
	DECLARE_CONVERTER(OPC_ALOAD_3, convert_aload_x, 2),
	DECLARE_CONVERTER(OPC_IALOAD, convert_xaload, 1),
	DECLARE_CONVERTER(OPC_LALOAD, convert_xaload, 1),
	DECLARE_CONVERTER(OPC_FALOAD, convert_xaload, 1),
	DECLARE_CONVERTER(OPC_DALOAD, convert_xaload, 1),
	DECLARE_CONVERTER(OPC_AALOAD, convert_xaload, 1),
	DECLARE_CONVERTER(OPC_BALOAD, convert_xaload, 1),
	DECLARE_CONVERTER(OPC_CALOAD, convert_xaload, 1),
	DECLARE_CONVERTER(OPC_SALOAD, convert_xaload, 1),
	DECLARE_CONVERTER(OPC_ISTORE, convert_istore, 1),
	DECLARE_CONVERTER(OPC_LSTORE, convert_lstore, 1),
	DECLARE_CONVERTER(OPC_FSTORE, convert_fstore, 1),
	DECLARE_CONVERTER(OPC_DSTORE, convert_dstore, 1),
	DECLARE_CONVERTER(OPC_ASTORE, convert_astore, 1),
};

struct statement *convert_bytecode_to_stmts(struct classblock *cb,
					    unsigned char *code,
					    unsigned long len, 
					    struct operand_stack *stack)
{
	struct converter *converter = &converters[code[0]];
	if (!converter || len < converter->require)
		return NULL;

	struct conversion_context context = {
		.cb = cb,
		.code = code,
		.len = len,
		.stack = stack
	};
	return converter->convert(&context);
}
