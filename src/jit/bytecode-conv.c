/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file contains functions for converting Java bytecode to immediate
 * language of the JIT compiler.
 */

#include <statement.h>
#include <byteorder.h>
#include <operand-stack.h>

#include <stdlib.h>

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

static struct statement *__convert_const(enum constant_type constant_type,
					 unsigned long long value,
					 struct operand_stack *stack)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		operand_set_const(stmt->s_left, constant_type, value);
		stack_push(stack, stmt->s_target->temporary);
	}
	return stmt;
}

static struct statement *convert_aconst_null(struct conversion_context *context)
{
	return __convert_const(CONST_REFERENCE, 0, context->stack);
}

static struct statement *convert_iconst(struct conversion_context *context)
{
	return __convert_const(CONST_INT, context->code[0] - OPC_ICONST_0,
			       context->stack);
}

static struct statement *convert_lconst(struct conversion_context *context)
{
	return __convert_const(CONST_LONG, context->code[0] - OPC_LCONST_0,
			       context->stack);
}

static struct statement *__convert_fconst(enum constant_type constant_type,
					  double value,
					  struct operand_stack *stack)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		operand_set_fconst(stmt->s_left, constant_type, value);
		stack_push(stack, stmt->s_target->temporary);
	}
	return stmt;
}

static struct statement *convert_fconst(struct conversion_context *context)
{
	return __convert_fconst(CONST_FLOAT, context->code[0] - OPC_FCONST_0,
				context->stack);
}

static struct statement *convert_dconst(struct conversion_context *context)
{
	return __convert_fconst(CONST_DOUBLE, context->code[0] - OPC_DCONST_0,
				context->stack);
}

static struct statement *convert_bipush(struct conversion_context *context)
{
	return __convert_const(CONST_INT, (char)context->code[1],
			       context->stack);
}

static struct statement *convert_sipush(struct conversion_context *context)
{
	return __convert_const(CONST_INT,
			       (short)be16_to_cpu(*(u2 *) & context->code[1]),
			       context->stack);
}

static struct statement *__convert_ldc(struct constant_pool *cp,
				       unsigned long cp_idx,
				       struct operand_stack *stack)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (!stmt)
		goto failed;

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
		operand_set_const(stmt->s_left, CONST_REFERENCE, entry);
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
		goto failed;
	}
	stack_push(stack, stmt->s_target->temporary);

	return stmt;
failed:
	free_stmt(stmt);
	return NULL;
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

static struct statement *convert_iload(struct conversion_context *context)
{
	return __convert_load(context->code[1], J_INT, context->stack);
}

static struct statement *convert_lload(struct conversion_context *context)
{
	return __convert_load(context->code[1], J_LONG, context->stack);
}

static struct statement *convert_fload(struct conversion_context *context)
{
	return __convert_load(context->code[1], J_FLOAT, context->stack);
}

static struct statement *convert_dload(struct conversion_context *context)
{
	return __convert_load(context->code[1], J_DOUBLE, context->stack);
}

static struct statement *convert_aload(struct conversion_context *context)
{
	return __convert_load(context->code[1], J_REFERENCE, context->stack);
}

static struct statement *convert_iload_n(struct conversion_context *context)
{
	return __convert_load(context->code[0] - OPC_ILOAD_0, J_INT,
			      context->stack);
}

static struct statement *convert_lload_n(struct conversion_context *context)
{
	return __convert_load(context->code[0] - OPC_LLOAD_0, J_LONG,
			      context->stack);
}

static struct statement *convert_fload_n(struct conversion_context *context)
{
	return __convert_load(context->code[0] - OPC_FLOAD_0, J_FLOAT,
			      context->stack);
}

static struct statement *convert_dload_n(struct conversion_context *context)
{
	return __convert_load(context->code[0] - OPC_DLOAD_0, J_DOUBLE,
			      context->stack);
}

static struct statement *convert_aload_n(struct conversion_context *context)
{
	return __convert_load(context->code[0] - OPC_ALOAD_0, J_REFERENCE,
			      context->stack);
}

static struct statement *convert_array_load(struct conversion_context *context)
{
	unsigned long index, arrayref;
	struct statement *assign, *arraycheck, *nullcheck;

	index = stack_pop(context->stack);
	arrayref = stack_pop(context->stack);

	assign = alloc_stmt(STMT_ASSIGN);
	if (!assign)
		goto failed;

	operand_set_arrayref(assign->s_left, arrayref, index);

	arraycheck = alloc_stmt(STMT_ARRAY_CHECK);
	if (!arraycheck)
		goto failed;

	operand_set_temporary(arraycheck->s_left, arrayref);
	operand_set_temporary(arraycheck->s_right, index);
	arraycheck->s_next = assign;

	nullcheck = alloc_stmt(STMT_NULL_CHECK);
	if (!nullcheck)
		goto failed;

	operand_set_temporary(nullcheck->s_left, arrayref);
	nullcheck->s_next = arraycheck;

	stack_push(context->stack, assign->s_target->temporary);

	return nullcheck;

failed:
	free_stmt(assign);
	free_stmt(arraycheck);
	free_stmt(nullcheck);
	return NULL;
}

static struct statement *__convert_store(enum jvm_type type,
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
	return __convert_store(J_INT, context->code[1], context->stack);
}

static struct statement *convert_lstore(struct conversion_context *context)
{
	return __convert_store(J_LONG, context->code[1], context->stack);
}

static struct statement *convert_fstore(struct conversion_context *context)
{
	return __convert_store(J_FLOAT, context->code[1], context->stack);
}

static struct statement *convert_dstore(struct conversion_context *context)
{
	return __convert_store(J_DOUBLE, context->code[1], context->stack);
}

static struct statement *convert_astore(struct conversion_context *context)
{
	return __convert_store(J_REFERENCE, context->code[1], context->stack);
}

static struct statement *convert_istore_n(struct conversion_context *context)
{
	return __convert_store(J_INT, context->code[0] - OPC_ISTORE_0,
			       context->stack);
}

static struct statement *convert_lstore_n(struct conversion_context *context)
{
	return __convert_store(J_LONG, context->code[0] - OPC_LSTORE_0,
			       context->stack);
}

static struct statement *convert_fstore_n(struct conversion_context *context)
{
	return __convert_store(J_FLOAT, context->code[0] - OPC_FSTORE_0,
			       context->stack);
}

static struct statement *convert_dstore_n(struct conversion_context *context)
{
	return __convert_store(J_DOUBLE, context->code[0] - OPC_DSTORE_0,
			       context->stack);
}

static struct statement *convert_astore_n(struct conversion_context *context)
{
	return __convert_store(J_REFERENCE, context->code[0] - OPC_ASTORE_0,
			       context->stack);
}

static struct statement *
convert_array_store(struct conversion_context *context)
{
	unsigned long value, index, arrayref;
	struct statement *assign, *arraycheck, *nullcheck;
	
	value = stack_pop(context->stack);
	index = stack_pop(context->stack);
	arrayref = stack_pop(context->stack);

	assign = alloc_stmt(STMT_ASSIGN);
	if (!assign)
		goto failed;

	operand_set_arrayref(assign->s_target, arrayref, index);
	operand_set_temporary(assign->s_left, value);

	arraycheck = alloc_stmt(STMT_ARRAY_CHECK);
	if (!arraycheck)
		goto failed;

	operand_set_temporary(arraycheck->s_left, arrayref);
	operand_set_temporary(arraycheck->s_right, index);
	arraycheck->s_next = assign;

	nullcheck = alloc_stmt(STMT_NULL_CHECK);
	if (!nullcheck)
		goto failed;

	operand_set_temporary(nullcheck->s_left, arrayref);
	nullcheck->s_next = arraycheck;

	return nullcheck;

 failed:
	free_stmt(assign);
	free_stmt(arraycheck);
	free_stmt(nullcheck);
	return NULL;
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
	DECLARE_CONVERTER(OPC_ILOAD_0, convert_iload_n, 1),
	DECLARE_CONVERTER(OPC_ILOAD_1, convert_iload_n, 1),
	DECLARE_CONVERTER(OPC_ILOAD_2, convert_iload_n, 1),
	DECLARE_CONVERTER(OPC_ILOAD_3, convert_iload_n, 1),
	DECLARE_CONVERTER(OPC_LLOAD_0, convert_lload_n, 1),
	DECLARE_CONVERTER(OPC_LLOAD_1, convert_lload_n, 1),
	DECLARE_CONVERTER(OPC_LLOAD_2, convert_lload_n, 1),
	DECLARE_CONVERTER(OPC_LLOAD_3, convert_lload_n, 1),
	DECLARE_CONVERTER(OPC_FLOAD_0, convert_fload_n, 1),
	DECLARE_CONVERTER(OPC_FLOAD_1, convert_fload_n, 1),
	DECLARE_CONVERTER(OPC_FLOAD_2, convert_fload_n, 1),
	DECLARE_CONVERTER(OPC_FLOAD_3, convert_fload_n, 1),
	DECLARE_CONVERTER(OPC_DLOAD_0, convert_dload_n, 1),
	DECLARE_CONVERTER(OPC_DLOAD_1, convert_dload_n, 1),
	DECLARE_CONVERTER(OPC_DLOAD_2, convert_dload_n, 1),
	DECLARE_CONVERTER(OPC_DLOAD_3, convert_dload_n, 1),
	DECLARE_CONVERTER(OPC_ALOAD_0, convert_aload_n, 1),
	DECLARE_CONVERTER(OPC_ALOAD_1, convert_aload_n, 1),
	DECLARE_CONVERTER(OPC_ALOAD_2, convert_aload_n, 1),
	DECLARE_CONVERTER(OPC_ALOAD_3, convert_aload_n, 1),
	DECLARE_CONVERTER(OPC_IALOAD, convert_array_load, 1),
	DECLARE_CONVERTER(OPC_LALOAD, convert_array_load, 1),
	DECLARE_CONVERTER(OPC_FALOAD, convert_array_load, 1),
	DECLARE_CONVERTER(OPC_DALOAD, convert_array_load, 1),
	DECLARE_CONVERTER(OPC_AALOAD, convert_array_load, 1),
	DECLARE_CONVERTER(OPC_BALOAD, convert_array_load, 1),
	DECLARE_CONVERTER(OPC_CALOAD, convert_array_load, 1),
	DECLARE_CONVERTER(OPC_SALOAD, convert_array_load, 1),
	DECLARE_CONVERTER(OPC_ISTORE, convert_istore, 1),
	DECLARE_CONVERTER(OPC_LSTORE, convert_lstore, 1),
	DECLARE_CONVERTER(OPC_FSTORE, convert_fstore, 1),
	DECLARE_CONVERTER(OPC_DSTORE, convert_dstore, 1),
	DECLARE_CONVERTER(OPC_ASTORE, convert_astore, 1),
	DECLARE_CONVERTER(OPC_ISTORE_0, convert_istore_n, 1),
	DECLARE_CONVERTER(OPC_ISTORE_1, convert_istore_n, 1),
	DECLARE_CONVERTER(OPC_ISTORE_2, convert_istore_n, 1),
	DECLARE_CONVERTER(OPC_ISTORE_3, convert_istore_n, 1),
	DECLARE_CONVERTER(OPC_LSTORE_0, convert_lstore_n, 1),
	DECLARE_CONVERTER(OPC_LSTORE_1, convert_lstore_n, 1),
	DECLARE_CONVERTER(OPC_LSTORE_2, convert_lstore_n, 1),
	DECLARE_CONVERTER(OPC_LSTORE_3, convert_lstore_n, 1),
	DECLARE_CONVERTER(OPC_FSTORE_0, convert_fstore_n, 1),
	DECLARE_CONVERTER(OPC_FSTORE_1, convert_fstore_n, 1),
	DECLARE_CONVERTER(OPC_FSTORE_2, convert_fstore_n, 1),
	DECLARE_CONVERTER(OPC_FSTORE_3, convert_fstore_n, 1),
	DECLARE_CONVERTER(OPC_DSTORE_0, convert_dstore_n, 1),
	DECLARE_CONVERTER(OPC_DSTORE_1, convert_dstore_n, 1),
	DECLARE_CONVERTER(OPC_DSTORE_2, convert_dstore_n, 1),
	DECLARE_CONVERTER(OPC_DSTORE_3, convert_dstore_n, 1),
	DECLARE_CONVERTER(OPC_ASTORE_0, convert_astore_n, 1),
	DECLARE_CONVERTER(OPC_ASTORE_1, convert_astore_n, 1),
	DECLARE_CONVERTER(OPC_ASTORE_2, convert_astore_n, 1),
	DECLARE_CONVERTER(OPC_ASTORE_3, convert_astore_n, 1),
	DECLARE_CONVERTER(OPC_IASTORE, convert_array_store, 1),
	DECLARE_CONVERTER(OPC_LASTORE, convert_array_store, 1),
	DECLARE_CONVERTER(OPC_FASTORE, convert_array_store, 1),
	DECLARE_CONVERTER(OPC_DASTORE, convert_array_store, 1),
	DECLARE_CONVERTER(OPC_AASTORE, convert_array_store, 1),
	DECLARE_CONVERTER(OPC_BASTORE, convert_array_store, 1),
	DECLARE_CONVERTER(OPC_CASTORE, convert_array_store, 1),
	DECLARE_CONVERTER(OPC_SASTORE, convert_array_store, 1),
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
