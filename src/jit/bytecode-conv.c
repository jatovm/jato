/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode to immediate
 * representation of the JIT compiler.
 */

#include <statement.h>
#include <byteorder.h>
#include <stack.h>

#include <stdlib.h>

struct compilation_unit {
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

static struct statement *convert_nop(struct compilation_unit *compilation_unit)
{
	return alloc_statement(STMT_NOP);
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

static struct statement *convert_aconst_null(struct compilation_unit
					     *compilation_unit)
{
	return __convert_const(J_REFERENCE, 0, compilation_unit->expr_stack);
}

static struct statement *convert_iconst(struct compilation_unit
					*compilation_unit)
{
	return __convert_const(J_INT, compilation_unit->code[0] - OPC_ICONST_0,
			       compilation_unit->expr_stack);
}

static struct statement *convert_lconst(struct compilation_unit
					*compilation_unit)
{
	return __convert_const(J_LONG, compilation_unit->code[0] - OPC_LCONST_0,
			       compilation_unit->expr_stack);
}

static struct statement *__convert_fconst(enum jvm_type jvm_type,
					  double value,
					  struct stack *expr_stack)
{
	struct expression *expr = fvalue_expr(jvm_type, value);
	if (expr)
		stack_push(expr_stack, expr);

	return 0;
}

static struct statement *convert_fconst(struct compilation_unit
					*compilation_unit)
{
	return __convert_fconst(J_FLOAT,
				compilation_unit->code[0] - OPC_FCONST_0,
				compilation_unit->expr_stack);
}

static struct statement *convert_dconst(struct compilation_unit
					*compilation_unit)
{
	return __convert_fconst(J_DOUBLE,
				compilation_unit->code[0] - OPC_DCONST_0,
				compilation_unit->expr_stack);
}

static struct statement *convert_bipush(struct compilation_unit
					*compilation_unit)
{
	return __convert_const(J_INT, (char)compilation_unit->code[1],
			       compilation_unit->expr_stack);
}

static struct statement *convert_sipush(struct compilation_unit
					*compilation_unit)
{
	return __convert_const(J_INT,
			       (short)be16_to_cpu(*(u2 *) & compilation_unit->
						  code[1]),
			       compilation_unit->expr_stack);
}

static struct statement *__convert_ldc(struct constant_pool *cp,
				       unsigned long cp_idx,
				       struct stack *expr_stack)
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
		expr = value_expr(J_REFERENCE, CP_LONG(cp, cp_idx));
		break;
	case CONSTANT_Long:
		expr = value_expr(J_LONG, CP_LONG(cp, cp_idx));
		break;
	case CONSTANT_Double:
		expr = fvalue_expr(J_DOUBLE, CP_DOUBLE(cp, cp_idx));
		break;
	default:
		expr = NULL;
	}

	if (expr)
		stack_push(expr_stack, expr);

	return NULL;
}

static struct statement *convert_ldc(struct compilation_unit *compilation_unit)
{
	return __convert_ldc(&compilation_unit->cb->constant_pool,
			     compilation_unit->code[1],
			     compilation_unit->expr_stack);
}

static struct statement *convert_ldc_w(struct compilation_unit
				       *compilation_unit)
{
	return __convert_ldc(&compilation_unit->cb->constant_pool,
			     be16_to_cpu(*(u2 *) & compilation_unit->code[1]),
			     compilation_unit->expr_stack);
}

static struct statement *convert_ldc2_w(struct compilation_unit
					*compilation_unit)
{
	return __convert_ldc(&compilation_unit->cb->constant_pool,
			     be16_to_cpu(*(u2 *) & compilation_unit->code[1]),
			     compilation_unit->expr_stack);
}

static struct statement *__convert_load(unsigned char index,
					enum jvm_type type,
					struct stack *expr_stack)
{
	struct statement *stmt = alloc_statement(STMT_ASSIGN);
	if (stmt) {
		stmt->right = local_expr(type, index);
		stmt->left = temporary_expr(type, alloc_temporary());
		stack_push(expr_stack, stmt->left);
	}
	return stmt;
}

static struct statement *convert_iload(struct compilation_unit
				       *compilation_unit)
{
	return __convert_load(compilation_unit->code[1], J_INT,
			      compilation_unit->expr_stack);
}

static struct statement *convert_lload(struct compilation_unit
				       *compilation_unit)
{
	return __convert_load(compilation_unit->code[1], J_LONG,
			      compilation_unit->expr_stack);
}

static struct statement *convert_fload(struct compilation_unit
				       *compilation_unit)
{
	return __convert_load(compilation_unit->code[1], J_FLOAT,
			      compilation_unit->expr_stack);
}

static struct statement *convert_dload(struct compilation_unit
				       *compilation_unit)
{
	return __convert_load(compilation_unit->code[1], J_DOUBLE,
			      compilation_unit->expr_stack);
}

static struct statement *convert_aload(struct compilation_unit
				       *compilation_unit)
{
	return __convert_load(compilation_unit->code[1], J_REFERENCE,
			      compilation_unit->expr_stack);
}

static struct statement *convert_iload_n(struct compilation_unit
					 *compilation_unit)
{
	return __convert_load(compilation_unit->code[0] - OPC_ILOAD_0, J_INT,
			      compilation_unit->expr_stack);
}

static struct statement *convert_lload_n(struct compilation_unit
					 *compilation_unit)
{
	return __convert_load(compilation_unit->code[0] - OPC_LLOAD_0, J_LONG,
			      compilation_unit->expr_stack);
}

static struct statement *convert_fload_n(struct compilation_unit
					 *compilation_unit)
{
	return __convert_load(compilation_unit->code[0] - OPC_FLOAD_0, J_FLOAT,
			      compilation_unit->expr_stack);
}

static struct statement *convert_dload_n(struct compilation_unit
					 *compilation_unit)
{
	return __convert_load(compilation_unit->code[0] - OPC_DLOAD_0, J_DOUBLE,
			      compilation_unit->expr_stack);
}

static struct statement *convert_aload_n(struct compilation_unit
					 *compilation_unit)
{
	return __convert_load(compilation_unit->code[0] - OPC_ALOAD_0,
			      J_REFERENCE, compilation_unit->expr_stack);
}

static struct statement *convert_array_load(struct compilation_unit
					    *compilation_unit,
					    enum jvm_type type)
{
	struct expression *index, *arrayref;
	struct statement *assign, *arraycheck, *nullcheck;

	index = stack_pop(compilation_unit->expr_stack);
	arrayref = stack_pop(compilation_unit->expr_stack);

	assign = alloc_statement(STMT_ASSIGN);
	if (!assign)
		goto failed;

	assign->right = array_deref_expr(type, arrayref, index);
	assign->left = temporary_expr(type, alloc_temporary());

	expr_get(assign->left);
	stack_push(compilation_unit->expr_stack, assign->left);

	arraycheck = alloc_statement(STMT_ARRAY_CHECK);
	if (!arraycheck)
		goto failed;

	expr_get(assign->right);
	arraycheck->expression = assign->right;
	arraycheck->next = assign;

	nullcheck = alloc_statement(STMT_NULL_CHECK);
	if (!nullcheck)
		goto failed;

	expr_get(arrayref);
	nullcheck->expression = arrayref;
	nullcheck->next = arraycheck;

	return nullcheck;

      failed:
	free_statement(assign);
	free_statement(arraycheck);
	free_statement(nullcheck);
	return NULL;
}

static struct statement *convert_iaload(struct compilation_unit
					*compilation_unit)
{
	return convert_array_load(compilation_unit, J_INT);
}

static struct statement *convert_laload(struct compilation_unit
					*compilation_unit)
{
	return convert_array_load(compilation_unit, J_LONG);
}

static struct statement *convert_faload(struct compilation_unit
					*compilation_unit)
{
	return convert_array_load(compilation_unit, J_FLOAT);
}

static struct statement *convert_daload(struct compilation_unit
					*compilation_unit)
{
	return convert_array_load(compilation_unit, J_DOUBLE);
}

static struct statement *convert_aaload(struct compilation_unit
					*compilation_unit)
{
	return convert_array_load(compilation_unit, J_REFERENCE);
}

static struct statement *convert_baload(struct compilation_unit
					*compilation_unit)
{
	return convert_array_load(compilation_unit, J_INT);
}

static struct statement *convert_caload(struct compilation_unit
					*compilation_unit)
{
	return convert_array_load(compilation_unit, J_CHAR);
}

static struct statement *convert_saload(struct compilation_unit
					*compilation_unit)
{
	return convert_array_load(compilation_unit, J_SHORT);
}

static struct statement *__convert_store(enum jvm_type type,
					 unsigned long index,
					 struct stack *expr_stack)
{
	struct statement *stmt = alloc_statement(STMT_ASSIGN);
	if (!stmt)
		goto failed;

	stmt->left = local_expr(type, index);
	stmt->right = stack_pop(expr_stack);
	return stmt;
      failed:
	free_statement(stmt);
	return NULL;
}

static struct statement *convert_istore(struct compilation_unit
					*compilation_unit)
{
	return __convert_store(J_INT, compilation_unit->code[1],
			       compilation_unit->expr_stack);
}

static struct statement *convert_lstore(struct compilation_unit
					*compilation_unit)
{
	return __convert_store(J_LONG, compilation_unit->code[1],
			       compilation_unit->expr_stack);
}

static struct statement *convert_fstore(struct compilation_unit
					*compilation_unit)
{
	return __convert_store(J_FLOAT, compilation_unit->code[1],
			       compilation_unit->expr_stack);
}

static struct statement *convert_dstore(struct compilation_unit
					*compilation_unit)
{
	return __convert_store(J_DOUBLE, compilation_unit->code[1],
			       compilation_unit->expr_stack);
}

static struct statement *convert_astore(struct compilation_unit
					*compilation_unit)
{
	return __convert_store(J_REFERENCE, compilation_unit->code[1],
			       compilation_unit->expr_stack);
}

static struct statement *convert_istore_n(struct compilation_unit
					  *compilation_unit)
{
	return __convert_store(J_INT, compilation_unit->code[0] - OPC_ISTORE_0,
			       compilation_unit->expr_stack);
}

static struct statement *convert_lstore_n(struct compilation_unit
					  *compilation_unit)
{
	return __convert_store(J_LONG, compilation_unit->code[0] - OPC_LSTORE_0,
			       compilation_unit->expr_stack);
}

static struct statement *convert_fstore_n(struct compilation_unit
					  *compilation_unit)
{
	return __convert_store(J_FLOAT,
			       compilation_unit->code[0] - OPC_FSTORE_0,
			       compilation_unit->expr_stack);
}

static struct statement *convert_dstore_n(struct compilation_unit
					  *compilation_unit)
{
	return __convert_store(J_DOUBLE,
			       compilation_unit->code[0] - OPC_DSTORE_0,
			       compilation_unit->expr_stack);
}

static struct statement *convert_astore_n(struct compilation_unit
					  *compilation_unit)
{
	return __convert_store(J_REFERENCE,
			       compilation_unit->code[0] - OPC_ASTORE_0,
			       compilation_unit->expr_stack);
}

static struct statement *convert_array_store(struct compilation_unit
					     *compilation_unit,
					     enum jvm_type type)
{
	struct expression *value, *index, *arrayref;
	struct statement *assign, *arraycheck, *nullcheck;

	value = stack_pop(compilation_unit->expr_stack);
	index = stack_pop(compilation_unit->expr_stack);
	arrayref = stack_pop(compilation_unit->expr_stack);

	assign = alloc_statement(STMT_ASSIGN);
	if (!assign)
		goto failed;

	assign->left = array_deref_expr(type, arrayref, index);
	assign->right = value;

	arraycheck = alloc_statement(STMT_ARRAY_CHECK);
	if (!arraycheck)
		goto failed;

	expr_get(assign->left);
	arraycheck->expression = assign->left;
	arraycheck->next = assign;

	nullcheck = alloc_statement(STMT_NULL_CHECK);
	if (!nullcheck)
		goto failed;

	expr_get(arrayref);
	nullcheck->expression = arrayref;
	nullcheck->next = arraycheck;

	return nullcheck;

      failed:
	free_statement(assign);
	free_statement(arraycheck);
	free_statement(nullcheck);
	return NULL;
}

static struct statement *convert_iastore(struct compilation_unit
					 *compilation_unit)
{
	return convert_array_store(compilation_unit, J_INT);
}

static struct statement *convert_lastore(struct compilation_unit
					 *compilation_unit)
{
	return convert_array_store(compilation_unit, J_LONG);
}

static struct statement *convert_fastore(struct compilation_unit
					 *compilation_unit)
{
	return convert_array_store(compilation_unit, J_FLOAT);
}

static struct statement *convert_dastore(struct compilation_unit
					 *compilation_unit)
{
	return convert_array_store(compilation_unit, J_DOUBLE);
}

static struct statement *convert_aastore(struct compilation_unit
					 *compilation_unit)
{
	return convert_array_store(compilation_unit, J_REFERENCE);
}

static struct statement *convert_bastore(struct compilation_unit
					 *compilation_unit)
{
	return convert_array_store(compilation_unit, J_INT);
}

static struct statement *convert_castore(struct compilation_unit
					 *compilation_unit)
{
	return convert_array_store(compilation_unit, J_CHAR);
}

static struct statement *convert_sastore(struct compilation_unit
					 *compilation_unit)
{
	return convert_array_store(compilation_unit, J_SHORT);
}

static struct statement *convert_pop(struct compilation_unit *compilation_unit)
{
	stack_pop(compilation_unit->expr_stack);
	return NULL;
}

static struct statement *convert_dup(struct compilation_unit *compilation_unit)
{
	void *value = stack_pop(compilation_unit->expr_stack);
	stack_push(compilation_unit->expr_stack, value);
	stack_push(compilation_unit->expr_stack, value);
	return NULL;
}

static struct statement *convert_dup_x1(struct compilation_unit
					*compilation_unit)
{
	void *value1 = stack_pop(compilation_unit->expr_stack);
	void *value2 = stack_pop(compilation_unit->expr_stack);
	stack_push(compilation_unit->expr_stack, value1);
	stack_push(compilation_unit->expr_stack, value2);
	stack_push(compilation_unit->expr_stack, value1);
	return NULL;
}

static struct statement *convert_dup_x2(struct compilation_unit
					*compilation_unit)
{
	void *value1 = stack_pop(compilation_unit->expr_stack);
	void *value2 = stack_pop(compilation_unit->expr_stack);
	void *value3 = stack_pop(compilation_unit->expr_stack);
	stack_push(compilation_unit->expr_stack, value1);
	stack_push(compilation_unit->expr_stack, value3);
	stack_push(compilation_unit->expr_stack, value2);
	stack_push(compilation_unit->expr_stack, value1);
	return NULL;
}

static struct statement *convert_swap(struct compilation_unit *compilation_unit)
{
	void *value1 = stack_pop(compilation_unit->expr_stack);
	void *value2 = stack_pop(compilation_unit->expr_stack);
	stack_push(compilation_unit->expr_stack, value1);
	stack_push(compilation_unit->expr_stack, value2);
	return NULL;
}

static struct statement *convert_binop(struct compilation_unit
				       *compilation_unit,
				       enum jvm_type jvm_type,
				       enum binary_operator binary_operator)
{
	struct expression *left, *right, *expr;

	right = stack_pop(compilation_unit->expr_stack);
	left = stack_pop(compilation_unit->expr_stack);

	expr = binop_expr(jvm_type, binary_operator, left, right);
	if (expr)
		stack_push(compilation_unit->expr_stack, expr);

	return NULL;
}

static struct statement *convert_iadd(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_INT, OP_ADD);
}

static struct statement *convert_ladd(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_LONG, OP_ADD);
}

static struct statement *convert_fadd(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_FLOAT, OP_ADD);
}

static struct statement *convert_dadd(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_DOUBLE, OP_ADD);
}

static struct statement *convert_isub(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_INT, OP_SUB);
}

static struct statement *convert_lsub(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_LONG, OP_SUB);
}

static struct statement *convert_fsub(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_FLOAT, OP_SUB);
}

static struct statement *convert_dsub(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_DOUBLE, OP_SUB);
}

static struct statement *convert_imul(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_INT, OP_MUL);
}

static struct statement *convert_lmul(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_LONG, OP_MUL);
}

static struct statement *convert_fmul(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_FLOAT, OP_MUL);
}

static struct statement *convert_dmul(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_DOUBLE, OP_MUL);
}

static struct statement *convert_idiv(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_INT, OP_DIV);
}

static struct statement *convert_ldiv(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_LONG, OP_DIV);
}

static struct statement *convert_fdiv(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_FLOAT, OP_DIV);
}

static struct statement *convert_ddiv(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_DOUBLE, OP_DIV);
}

static struct statement *convert_irem(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_INT, OP_REM);
}

static struct statement *convert_lrem(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_LONG, OP_REM);
}

static struct statement *convert_frem(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_FLOAT, OP_REM);
}

static struct statement *convert_drem(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_DOUBLE, OP_REM);
}

static struct statement *convert_unary_op(struct compilation_unit
					  *compilation_unit,
					  enum jvm_type jvm_type,
					  enum unary_operator unary_operator)
{
	struct expression *expression, *expr;

	expression = stack_pop(compilation_unit->expr_stack);

	expr = unary_op_expr(jvm_type, unary_operator, expression);
	if (expr)
		stack_push(compilation_unit->expr_stack, expr);

	return NULL;
}

static struct statement *convert_ineg(struct compilation_unit *compilation_unit)
{
	return convert_unary_op(compilation_unit, J_INT, OP_NEG);
}

static struct statement *convert_lneg(struct compilation_unit *compilation_unit)
{
	return convert_unary_op(compilation_unit, J_LONG, OP_NEG);
}

static struct statement *convert_fneg(struct compilation_unit *compilation_unit)
{
	return convert_unary_op(compilation_unit, J_FLOAT, OP_NEG);
}

static struct statement *convert_dneg(struct compilation_unit *compilation_unit)
{
	return convert_unary_op(compilation_unit, J_DOUBLE, OP_NEG);
}

static struct statement *convert_ishl(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_INT, OP_SHL);
}

static struct statement *convert_lshl(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_LONG, OP_SHL);
}

static struct statement *convert_ishr(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_INT, OP_SHR);
}

static struct statement *convert_lshr(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_LONG, OP_SHR);
}

static struct statement *convert_iand(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_INT, OP_AND);
}

static struct statement *convert_land(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_LONG, OP_AND);
}

static struct statement *convert_ior(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_INT, OP_OR);
}

static struct statement *convert_lor(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_LONG, OP_OR);
}

static struct statement *convert_ixor(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_INT, OP_XOR);
}

static struct statement *convert_lxor(struct compilation_unit *compilation_unit)
{
	return convert_binop(compilation_unit, J_LONG, OP_XOR);
}

static struct statement *convert_iinc(struct compilation_unit *compilation_unit)
{
	struct statement *assign;
	struct expression *local_expression, *binop_expression,
	    *const_expression;

	assign = alloc_statement(STMT_ASSIGN);
	if (!assign)
		goto failed;

	local_expression = local_expr(J_INT, compilation_unit->code[1]);
	if (!local_expression)
		goto failed;

	assign->left = local_expression;

	const_expression = value_expr(J_INT, compilation_unit->code[2]);
	if (!const_expression)
		goto failed;

	expr_get(local_expression);

	binop_expression = binop_expr(J_INT, OP_ADD, local_expression,
				      const_expression);
	if (!binop_expression) {
		expr_put(local_expression);
		expr_put(const_expression);
		goto failed;
	}

	assign->right = binop_expression;

	return assign;

      failed:
	free_statement(assign);
	return NULL;
}

static struct statement *convert_conversion(struct compilation_unit *compilation_unit,
					    enum jvm_type to_type)
{
	struct expression *from_expression, *conversion_expression;

	from_expression = stack_pop(compilation_unit->expr_stack);

	conversion_expression = conversion_expr(to_type, from_expression);
	if (conversion_expression)
		stack_push(compilation_unit->expr_stack,
			   conversion_expression);

	return NULL;
}

static struct statement *convert_i2l(struct compilation_unit *compilation_unit)
{
	return convert_conversion(compilation_unit, J_LONG);
}

static struct statement *convert_i2f(struct compilation_unit *compilation_unit)
{
	return convert_conversion(compilation_unit, J_FLOAT);
}

static struct statement *convert_i2d(struct compilation_unit *compilation_unit)
{
	return convert_conversion(compilation_unit, J_DOUBLE);
}

static struct statement *convert_l2i(struct compilation_unit *compilation_unit)
{
	return convert_conversion(compilation_unit, J_INT);
}

static struct statement *convert_l2f(struct compilation_unit *compilation_unit)
{
	return convert_conversion(compilation_unit, J_FLOAT);
}

static struct statement *convert_l2d(struct compilation_unit *compilation_unit)
{
	return convert_conversion(compilation_unit, J_DOUBLE);
}

static struct statement *convert_f2i(struct compilation_unit *compilation_unit)
{
	return convert_conversion(compilation_unit, J_INT);
}

static struct statement *convert_f2l(struct compilation_unit *compilation_unit)
{
	return convert_conversion(compilation_unit, J_LONG);
}

static struct statement *convert_f2d(struct compilation_unit *compilation_unit)
{
	return convert_conversion(compilation_unit, J_DOUBLE);
}

static struct statement *convert_d2i(struct compilation_unit *compilation_unit)
{
	return convert_conversion(compilation_unit, J_INT);
}

static struct statement *convert_d2l(struct compilation_unit *compilation_unit)
{
	return convert_conversion(compilation_unit, J_LONG);
}

static struct statement *convert_d2f(struct compilation_unit *compilation_unit)
{
	return convert_conversion(compilation_unit, J_FLOAT);
}

static struct statement *convert_i2b(struct compilation_unit *compilation_unit)
{
	return convert_conversion(compilation_unit, J_BYTE);
}

static struct statement *convert_i2c(struct compilation_unit *compilation_unit)
{
	return convert_conversion(compilation_unit, J_CHAR);
}

static struct statement *convert_i2s(struct compilation_unit *compilation_unit)
{
	return convert_conversion(compilation_unit, J_SHORT);
}

typedef struct statement *(*convert_fn_t) (struct compilation_unit *);

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
	DECLARE_CONVERTER(OPC_INEG, convert_ineg, 1),
	DECLARE_CONVERTER(OPC_LNEG, convert_lneg, 1),
	DECLARE_CONVERTER(OPC_FNEG, convert_fneg, 1),
	DECLARE_CONVERTER(OPC_DNEG, convert_dneg, 1),
	DECLARE_CONVERTER(OPC_ISHL, convert_ishl, 1),
	DECLARE_CONVERTER(OPC_LSHL, convert_lshl, 1),
	DECLARE_CONVERTER(OPC_ISHR, convert_ishr, 1),
	DECLARE_CONVERTER(OPC_LSHR, convert_lshr, 1),
	DECLARE_CONVERTER(OPC_IAND, convert_iand, 1),
	DECLARE_CONVERTER(OPC_LAND, convert_land, 1),
	DECLARE_CONVERTER(OPC_IOR, convert_ior, 1),
	DECLARE_CONVERTER(OPC_LOR, convert_lor, 1),
	DECLARE_CONVERTER(OPC_IXOR, convert_ixor, 1),
	DECLARE_CONVERTER(OPC_LXOR, convert_lxor, 1),
	DECLARE_CONVERTER(OPC_IINC, convert_iinc, 3),
	DECLARE_CONVERTER(OPC_I2L, convert_i2l, 1),
	DECLARE_CONVERTER(OPC_I2F, convert_i2f, 1),
	DECLARE_CONVERTER(OPC_I2D, convert_i2d, 1),
	DECLARE_CONVERTER(OPC_L2I, convert_l2i, 1),
	DECLARE_CONVERTER(OPC_L2F, convert_l2f, 1),
	DECLARE_CONVERTER(OPC_L2D, convert_l2d, 1),
	DECLARE_CONVERTER(OPC_F2I, convert_f2i, 1),
	DECLARE_CONVERTER(OPC_F2L, convert_f2l, 1),
	DECLARE_CONVERTER(OPC_F2D, convert_f2d, 1),
	DECLARE_CONVERTER(OPC_D2I, convert_d2i, 1),
	DECLARE_CONVERTER(OPC_D2L, convert_d2l, 1),
	DECLARE_CONVERTER(OPC_D2F, convert_d2f, 1),
	DECLARE_CONVERTER(OPC_I2B, convert_i2b, 1),
	DECLARE_CONVERTER(OPC_I2C, convert_i2c, 1),
	DECLARE_CONVERTER(OPC_I2S, convert_i2s, 1),
};

struct statement *convert_bytecode_to_stmts(struct classblock *cb,
					    unsigned char *code,
					    unsigned long len,
					    struct stack *expr_stack)
{
	struct converter *converter = &converters[code[0]];
	if (!converter || len < converter->require)
		return NULL;

	struct compilation_unit compilation_unit = {
		.cb = cb,
		.code = code,
		.len = len,
		.expr_stack = expr_stack
	};
	return converter->convert(&compilation_unit);
}
