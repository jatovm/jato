/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file contains functions for converting Java bytecode to immediate
 * representation of the JIT compiler.
 */

#include <statement.h>
#include <byteorder.h>
#include <stack.h>

#include <stdlib.h>

struct conversion_context {
	struct classblock *cb;
	unsigned char *code;
	unsigned long len;
	struct stack *expr_stack;
};

static unsigned long alloc_temporary(void)
{
	static unsigned long temporary;
	return ++temporary;
}

static struct statement *convert_nop(struct conversion_context *context)
{
	return alloc_stmt(STMT_NOP);
}

static struct statement *__convert_const(enum jvm_type jvm_type,
					 unsigned long long value,
					 struct stack *expr_stack)
{
	struct expression *expr = value_expr(jvm_type, value);
	if (expr)
		stack_push(expr_stack, expr);

	return NULL;
}

static struct statement *convert_aconst_null(struct conversion_context *context)
{
	return __convert_const(J_REFERENCE, 0, context->expr_stack);
}

static struct statement *convert_iconst(struct conversion_context *context)
{
	return __convert_const(J_INT, context->code[0] - OPC_ICONST_0,
			       context->expr_stack);
}

static struct statement *convert_lconst(struct conversion_context *context)
{
	return __convert_const(J_LONG, context->code[0] - OPC_LCONST_0,
			       context->expr_stack);
}

static struct statement *__convert_fconst(enum jvm_type jvm_type,
					  double value, struct stack *expr_stack)
{
	struct expression *expr = fvalue_expr(jvm_type, value);
	if (expr)
		stack_push(expr_stack, expr);

	return 0;
}

static struct statement *convert_fconst(struct conversion_context *context)
{
	return __convert_fconst(J_FLOAT, context->code[0] - OPC_FCONST_0,
				context->expr_stack);
}

static struct statement *convert_dconst(struct conversion_context *context)
{
	return __convert_fconst(J_DOUBLE, context->code[0] - OPC_DCONST_0,
				context->expr_stack);
}

static struct statement *convert_bipush(struct conversion_context *context)
{
	return __convert_const(J_INT, (char)context->code[1], context->expr_stack);
}

static struct statement *convert_sipush(struct conversion_context *context)
{
	return __convert_const(J_INT,
			       (short)be16_to_cpu(*(u2 *) & context->code[1]),
			       context->expr_stack);
}

static struct statement *__convert_ldc(struct constant_pool *cp,
				       unsigned long cp_idx,
				       struct stack *expr_stack)
{
	struct expression *expr;

	u1 type = CP_TYPE(cp, cp_idx);
	ConstantPoolEntry entry = be64_to_cpu(CP_INFO(cp, cp_idx));
	switch (type) {
	case CONSTANT_Integer:
		expr = value_expr(J_INT, entry);
		break;
	case CONSTANT_Float:
		expr = fvalue_expr(J_FLOAT, *(float *)&entry);
		break;
	case CONSTANT_String:
		expr = value_expr(J_REFERENCE, entry);
		break;
	case CONSTANT_Long:
		expr = value_expr(J_LONG, entry);
		break;
	case CONSTANT_Double:
		expr = fvalue_expr(J_DOUBLE, *(double *)&entry);
		break;
	default:
		expr = NULL;
	}

	if (expr)
		stack_push(expr_stack, expr);

	return NULL;
}

static struct statement *convert_ldc(struct conversion_context *context)
{
	return __convert_ldc(&context->cb->constant_pool, context->code[1],
			     context->expr_stack);
}

static struct statement *convert_ldc_w(struct conversion_context *context)
{
	return __convert_ldc(&context->cb->constant_pool,
			     be16_to_cpu(*(u2 *) & context->code[1]),
			     context->expr_stack);
}

static struct statement *convert_ldc2_w(struct conversion_context *context)
{
	return __convert_ldc(&context->cb->constant_pool,
			     be16_to_cpu(*(u2 *) & context->code[1]),
			     context->expr_stack);
}

static struct statement *__convert_load(unsigned char index,
					enum jvm_type type, struct stack *expr_stack)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (stmt) {
		stmt->s_left = local_expr(type, index);
		stmt->s_target = temporary_expr(type, alloc_temporary());
		stack_push(expr_stack, stmt->s_target);
	}
	return stmt;
}

static struct statement *convert_iload(struct conversion_context *context)
{
	return __convert_load(context->code[1], J_INT, context->expr_stack);
}

static struct statement *convert_lload(struct conversion_context *context)
{
	return __convert_load(context->code[1], J_LONG, context->expr_stack);
}

static struct statement *convert_fload(struct conversion_context *context)
{
	return __convert_load(context->code[1], J_FLOAT, context->expr_stack);
}

static struct statement *convert_dload(struct conversion_context *context)
{
	return __convert_load(context->code[1], J_DOUBLE, context->expr_stack);
}

static struct statement *convert_aload(struct conversion_context *context)
{
	return __convert_load(context->code[1], J_REFERENCE, context->expr_stack);
}

static struct statement *convert_iload_n(struct conversion_context *context)
{
	return __convert_load(context->code[0] - OPC_ILOAD_0, J_INT,
			      context->expr_stack);
}

static struct statement *convert_lload_n(struct conversion_context *context)
{
	return __convert_load(context->code[0] - OPC_LLOAD_0, J_LONG,
			      context->expr_stack);
}

static struct statement *convert_fload_n(struct conversion_context *context)
{
	return __convert_load(context->code[0] - OPC_FLOAD_0, J_FLOAT,
			      context->expr_stack);
}

static struct statement *convert_dload_n(struct conversion_context *context)
{
	return __convert_load(context->code[0] - OPC_DLOAD_0, J_DOUBLE,
			      context->expr_stack);
}

static struct statement *convert_aload_n(struct conversion_context *context)
{
	return __convert_load(context->code[0] - OPC_ALOAD_0, J_REFERENCE,
			      context->expr_stack);
}

static struct statement *convert_array_load(struct conversion_context *context,
					    enum jvm_type type)
{
	struct expression *index, *arrayref;
	struct statement *assign, *arraycheck, *nullcheck;

	index = stack_pop(context->expr_stack);
	arrayref = stack_pop(context->expr_stack);

	assign = alloc_stmt(STMT_ASSIGN);
	if (!assign)
		goto failed;

	assign->s_left = array_deref_expr(type, arrayref, index);
	assign->s_target = temporary_expr(type, alloc_temporary());

	expr_get(assign->s_target);
	stack_push(context->expr_stack, assign->s_target);

	arraycheck = alloc_stmt(STMT_ARRAY_CHECK);
	if (!arraycheck)
		goto failed;

	expr_get(arrayref);
	expr_get(index);
	arraycheck->s_left = array_deref_expr(type, arrayref, index);
	arraycheck->s_next = assign;

	nullcheck = alloc_stmt(STMT_NULL_CHECK);
	if (!nullcheck)
		goto failed;

	expr_get(arrayref);
	nullcheck->s_left = arrayref;
	nullcheck->s_next = arraycheck;

	return nullcheck;

      failed:
	free_stmt(assign);
	free_stmt(arraycheck);
	free_stmt(nullcheck);
	return NULL;
}

static struct statement *convert_iaload(struct conversion_context *context)
{
	return convert_array_load(context, J_INT);
}

static struct statement *convert_laload(struct conversion_context *context)
{
	return convert_array_load(context, J_LONG);
}

static struct statement *convert_faload(struct conversion_context *context)
{
	return convert_array_load(context, J_FLOAT);
}

static struct statement *convert_daload(struct conversion_context *context)
{
	return convert_array_load(context, J_DOUBLE);
}

static struct statement *convert_aaload(struct conversion_context *context)
{
	return convert_array_load(context, J_REFERENCE);
}

static struct statement *convert_baload(struct conversion_context *context)
{
	return convert_array_load(context, J_INT);
}

static struct statement *convert_caload(struct conversion_context *context)
{
	return convert_array_load(context, J_CHAR);
}

static struct statement *convert_saload(struct conversion_context *context)
{
	return convert_array_load(context, J_SHORT);
}

static struct statement *__convert_store(enum jvm_type type,
					 unsigned long index,
					 struct stack *expr_stack)
{
	struct statement *stmt = alloc_stmt(STMT_ASSIGN);
	if (!stmt)
		goto failed;

	stmt->s_target = local_expr(type, index);
	stmt->s_left = stack_pop(expr_stack);
	return stmt;
      failed:
	free_stmt(stmt);
	return NULL;
}

static struct statement *convert_istore(struct conversion_context *context)
{
	return __convert_store(J_INT, context->code[1], context->expr_stack);
}

static struct statement *convert_lstore(struct conversion_context *context)
{
	return __convert_store(J_LONG, context->code[1], context->expr_stack);
}

static struct statement *convert_fstore(struct conversion_context *context)
{
	return __convert_store(J_FLOAT, context->code[1], context->expr_stack);
}

static struct statement *convert_dstore(struct conversion_context *context)
{
	return __convert_store(J_DOUBLE, context->code[1], context->expr_stack);
}

static struct statement *convert_astore(struct conversion_context *context)
{
	return __convert_store(J_REFERENCE, context->code[1], context->expr_stack);
}

static struct statement *convert_istore_n(struct conversion_context *context)
{
	return __convert_store(J_INT, context->code[0] - OPC_ISTORE_0,
			       context->expr_stack);
}

static struct statement *convert_lstore_n(struct conversion_context *context)
{
	return __convert_store(J_LONG, context->code[0] - OPC_LSTORE_0,
			       context->expr_stack);
}

static struct statement *convert_fstore_n(struct conversion_context *context)
{
	return __convert_store(J_FLOAT, context->code[0] - OPC_FSTORE_0,
			       context->expr_stack);
}

static struct statement *convert_dstore_n(struct conversion_context *context)
{
	return __convert_store(J_DOUBLE, context->code[0] - OPC_DSTORE_0,
			       context->expr_stack);
}

static struct statement *convert_astore_n(struct conversion_context *context)
{
	return __convert_store(J_REFERENCE, context->code[0] - OPC_ASTORE_0,
			       context->expr_stack);
}

static struct statement *convert_array_store(struct conversion_context *context,
					     enum jvm_type type)
{
	struct expression *value, *index, *arrayref;
	struct statement *assign, *arraycheck, *nullcheck;

	value = stack_pop(context->expr_stack);
	index = stack_pop(context->expr_stack);
	arrayref = stack_pop(context->expr_stack);

	assign = alloc_stmt(STMT_ASSIGN);
	if (!assign)
		goto failed;

	assign->s_target = array_deref_expr(type, arrayref, index);
	assign->s_left = value;

	arraycheck = alloc_stmt(STMT_ARRAY_CHECK);
	if (!arraycheck)
		goto failed;

	expr_get(arrayref);
	expr_get(index);
	arraycheck->s_left = array_deref_expr(type, arrayref, index);
	arraycheck->s_next = assign;

	nullcheck = alloc_stmt(STMT_NULL_CHECK);
	if (!nullcheck)
		goto failed;

	expr_get(arrayref);
	nullcheck->s_left = arrayref;
	nullcheck->s_next = arraycheck;

	return nullcheck;

      failed:
	free_stmt(assign);
	free_stmt(arraycheck);
	free_stmt(nullcheck);
	return NULL;
}

static struct statement *convert_iastore(struct conversion_context *context)
{
	return convert_array_store(context, J_INT);
}

static struct statement *convert_lastore(struct conversion_context *context)
{
	return convert_array_store(context, J_LONG);
}

static struct statement *convert_fastore(struct conversion_context *context)
{
	return convert_array_store(context, J_FLOAT);
}

static struct statement *convert_dastore(struct conversion_context *context)
{
	return convert_array_store(context, J_DOUBLE);
}

static struct statement *convert_aastore(struct conversion_context *context)
{
	return convert_array_store(context, J_REFERENCE);
}

static struct statement *convert_bastore(struct conversion_context *context)
{
	return convert_array_store(context, J_INT);
}

static struct statement *convert_castore(struct conversion_context *context)
{
	return convert_array_store(context, J_CHAR);
}

static struct statement *convert_sastore(struct conversion_context *context)
{
	return convert_array_store(context, J_SHORT);
}

static struct statement *convert_pop(struct conversion_context *context)
{
	stack_pop(context->expr_stack);
	return NULL;
}

static struct statement *convert_dup(struct conversion_context *context)
{
	void *value = stack_pop(context->expr_stack);
	stack_push(context->expr_stack, value);
	stack_push(context->expr_stack, value);
	return NULL;
}

static struct statement *convert_dup_x1(struct conversion_context *context)
{
	void *value1 = stack_pop(context->expr_stack);
	void *value2 = stack_pop(context->expr_stack);
	stack_push(context->expr_stack, value1);
	stack_push(context->expr_stack, value2);
	stack_push(context->expr_stack, value1);
	return NULL;
}

static struct statement *convert_dup_x2(struct conversion_context *context)
{
	void *value1 = stack_pop(context->expr_stack);
	void *value2 = stack_pop(context->expr_stack);
	void *value3 = stack_pop(context->expr_stack);
	stack_push(context->expr_stack, value1);
	stack_push(context->expr_stack, value3);
	stack_push(context->expr_stack, value2);
	stack_push(context->expr_stack, value1);
	return NULL;
}

static struct statement *convert_swap(struct conversion_context *context)
{
	void *value1 = stack_pop(context->expr_stack);
	void *value2 = stack_pop(context->expr_stack);
	stack_push(context->expr_stack, value1);
	stack_push(context->expr_stack, value2);
	return NULL;
}

static struct statement *convert_binop(struct conversion_context *context,
				       enum jvm_type jvm_type,
				       enum operator operator)
{
	struct expression *left, *right, *expr;

	right = stack_pop(context->expr_stack);
	left = stack_pop(context->expr_stack);

	expr = binop_expr(jvm_type, operator, left, right);
	if (expr)
		stack_push(context->expr_stack, expr);
	
	return NULL;
}

static struct statement *convert_iadd(struct conversion_context *context)
{
	return convert_binop(context, J_INT, OP_ADD);
}

static struct statement *convert_ladd(struct conversion_context *context)
{
	return convert_binop(context, J_LONG, OP_ADD);
}

static struct statement *convert_fadd(struct conversion_context *context)
{
	return convert_binop(context, J_FLOAT, OP_ADD);
}

static struct statement *convert_dadd(struct conversion_context *context)
{
	return convert_binop(context, J_DOUBLE, OP_ADD);
}

static struct statement *convert_isub(struct conversion_context *context)
{
	return convert_binop(context, J_INT, OP_SUB);
}

static struct statement *convert_lsub(struct conversion_context *context)
{
	return convert_binop(context, J_LONG, OP_SUB);
}

static struct statement *convert_fsub(struct conversion_context *context)
{
	return convert_binop(context, J_FLOAT, OP_SUB);
}

static struct statement *convert_dsub(struct conversion_context *context)
{
	return convert_binop(context, J_DOUBLE, OP_SUB);
}

static struct statement *convert_imul(struct conversion_context *context)
{
	return convert_binop(context, J_INT, OP_MUL);
}

static struct statement *convert_lmul(struct conversion_context *context)
{
	return convert_binop(context, J_LONG, OP_MUL);
}

static struct statement *convert_fmul(struct conversion_context *context)
{
	return convert_binop(context, J_FLOAT, OP_MUL);
}

static struct statement *convert_dmul(struct conversion_context *context)
{
	return convert_binop(context, J_DOUBLE, OP_MUL);
}

static struct statement *convert_idiv(struct conversion_context *context)
{
	return convert_binop(context, J_INT, OP_DIV);
}

static struct statement *convert_ldiv(struct conversion_context *context)
{
	return convert_binop(context, J_LONG, OP_DIV);
}

static struct statement *convert_fdiv(struct conversion_context *context)
{
	return convert_binop(context, J_FLOAT, OP_DIV);
}

static struct statement *convert_ddiv(struct conversion_context *context)
{
	return convert_binop(context, J_DOUBLE, OP_DIV);
}

static struct statement *convert_irem(struct conversion_context *context)
{
	return convert_binop(context, J_INT, OP_REM);
}

static struct statement *convert_lrem(struct conversion_context *context)
{
	return convert_binop(context, J_LONG, OP_REM);
}

static struct statement *convert_frem(struct conversion_context *context)
{
	return convert_binop(context, J_FLOAT, OP_REM);
}

static struct statement *convert_drem(struct conversion_context *context)
{
	return convert_binop(context, J_DOUBLE, OP_REM);
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
	DECLARE_CONVERTER(OPC_IALOAD, convert_iaload, 1),
	DECLARE_CONVERTER(OPC_LALOAD, convert_laload, 1),
	DECLARE_CONVERTER(OPC_FALOAD, convert_faload, 1),
	DECLARE_CONVERTER(OPC_DALOAD, convert_daload, 1),
	DECLARE_CONVERTER(OPC_AALOAD, convert_aaload, 1),
	DECLARE_CONVERTER(OPC_BALOAD, convert_baload, 1),
	DECLARE_CONVERTER(OPC_CALOAD, convert_caload, 1),
	DECLARE_CONVERTER(OPC_SALOAD, convert_saload, 1),
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
	DECLARE_CONVERTER(OPC_IASTORE, convert_iastore, 1),
	DECLARE_CONVERTER(OPC_LASTORE, convert_lastore, 1),
	DECLARE_CONVERTER(OPC_FASTORE, convert_fastore, 1),
	DECLARE_CONVERTER(OPC_DASTORE, convert_dastore, 1),
	DECLARE_CONVERTER(OPC_AASTORE, convert_aastore, 1),
	DECLARE_CONVERTER(OPC_BASTORE, convert_bastore, 1),
	DECLARE_CONVERTER(OPC_CASTORE, convert_castore, 1),
	DECLARE_CONVERTER(OPC_SASTORE, convert_sastore, 1),
	DECLARE_CONVERTER(OPC_POP, convert_pop, 1),
	DECLARE_CONVERTER(OPC_POP2, convert_pop, 1),
	DECLARE_CONVERTER(OPC_DUP, convert_dup, 1),
	DECLARE_CONVERTER(OPC_DUP_X1, convert_dup_x1, 1),
	DECLARE_CONVERTER(OPC_DUP_X2, convert_dup_x2, 1),
	DECLARE_CONVERTER(OPC_DUP2, convert_dup, 1),
	DECLARE_CONVERTER(OPC_DUP2_X1, convert_dup_x1, 1),
	DECLARE_CONVERTER(OPC_DUP2_X2, convert_dup_x2, 1),
	DECLARE_CONVERTER(OPC_SWAP, convert_swap, 1),
	DECLARE_CONVERTER(OPC_IADD, convert_iadd, 1),
	DECLARE_CONVERTER(OPC_LADD, convert_ladd, 1),
	DECLARE_CONVERTER(OPC_FADD, convert_fadd, 1),
	DECLARE_CONVERTER(OPC_DADD, convert_dadd, 1),
	DECLARE_CONVERTER(OPC_ISUB, convert_isub, 1),
	DECLARE_CONVERTER(OPC_LSUB, convert_lsub, 1),
	DECLARE_CONVERTER(OPC_FSUB, convert_fsub, 1),
	DECLARE_CONVERTER(OPC_DSUB, convert_dsub, 1),
	DECLARE_CONVERTER(OPC_IMUL, convert_imul, 1),
	DECLARE_CONVERTER(OPC_LMUL, convert_lmul, 1),
	DECLARE_CONVERTER(OPC_FMUL, convert_fmul, 1),
	DECLARE_CONVERTER(OPC_DMUL, convert_dmul, 1),
	DECLARE_CONVERTER(OPC_IDIV, convert_idiv, 1),
	DECLARE_CONVERTER(OPC_LDIV, convert_ldiv, 1),
	DECLARE_CONVERTER(OPC_FDIV, convert_fdiv, 1),
	DECLARE_CONVERTER(OPC_DDIV, convert_ddiv, 1),
	DECLARE_CONVERTER(OPC_IREM, convert_irem, 1),
	DECLARE_CONVERTER(OPC_LREM, convert_lrem, 1),
	DECLARE_CONVERTER(OPC_FREM, convert_frem, 1),
	DECLARE_CONVERTER(OPC_DREM, convert_drem, 1),
};

struct statement *convert_bytecode_to_stmts(struct classblock *cb,
					    unsigned char *code,
					    unsigned long len,
					    struct stack *expr_stack)
{
	struct converter *converter = &converters[code[0]];
	if (!converter || len < converter->require)
		return NULL;

	struct conversion_context context = {
		.cb = cb,
		.code = code,
		.len = len,
		.expr_stack = expr_stack
	};
	return converter->convert(&context);
}
