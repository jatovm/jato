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
#include <jit-compiler.h>
#include <bytecodes.h>

#include <stdlib.h>

static unsigned long alloc_temporary(void)
{
	static unsigned long temporary;
	return ++temporary;
}

static struct statement *convert_nop(struct compilation_unit *cu,
				     struct basic_block *bb)
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

static struct statement *convert_aconst_null(struct compilation_unit *cu,
					     struct basic_block *bb)
{
	return __convert_const(J_REFERENCE, 0, cu->expr_stack);
}

static struct statement *convert_iconst(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return __convert_const(J_INT, cu->code[0] - OPC_ICONST_0,
			       cu->expr_stack);
}

static struct statement *convert_lconst(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return __convert_const(J_LONG, cu->code[0] - OPC_LCONST_0,
			       cu->expr_stack);
}

static struct statement *__convert_fconst(enum jvm_type jvm_type,
					  double value,
					  struct stack *expr_stack)
{
	struct expression *expr = fvalue_expr(jvm_type, value);
	if (expr)
		stack_push(expr_stack, expr);

	return NULL;
}

static struct statement *convert_fconst(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return __convert_fconst(J_FLOAT,
				cu->code[0] - OPC_FCONST_0, cu->expr_stack);
}

static struct statement *convert_dconst(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return __convert_fconst(J_DOUBLE,
				cu->code[0] - OPC_DCONST_0, cu->expr_stack);
}

static struct statement *convert_bipush(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return __convert_const(J_INT, (char)cu->code[1], cu->expr_stack);
}

static struct statement *convert_sipush(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return __convert_const(J_INT,
			       (short)be16_to_cpu(*(u2 *) & cu->code[1]),
			       cu->expr_stack);
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

static struct statement *convert_ldc(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	return __convert_ldc(&cu->cb->constant_pool,
			     cu->code[1], cu->expr_stack);
}

static struct statement *convert_ldc_w(struct compilation_unit *cu,
				       struct basic_block *bb)
{
	return __convert_ldc(&cu->cb->constant_pool,
			     be16_to_cpu(*(u2 *) & cu->code[1]),
			     cu->expr_stack);
}

static struct statement *convert_ldc2_w(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return __convert_ldc(&cu->cb->constant_pool,
			     be16_to_cpu(*(u2 *) & cu->code[1]),
			     cu->expr_stack);
}

static struct statement *convert_load(struct compilation_unit *cu,
				      struct basic_block *bb,
				      unsigned char index,
				      enum jvm_type type)
{
	struct expression *expr;

	expr = local_expr(type, index);
	if (expr)
		stack_push(cu->expr_stack, expr);

	return NULL;
}

static struct statement *convert_iload(struct compilation_unit *cu,
				       struct basic_block *bb)
{
	return convert_load(cu, bb, cu->code[1], J_INT);
}

static struct statement *convert_lload(struct compilation_unit *cu,
				       struct basic_block *bb)
{
	return convert_load(cu, bb, cu->code[1], J_LONG);
}

static struct statement *convert_fload(struct compilation_unit *cu,
				       struct basic_block *bb)
{
	return convert_load(cu, bb, cu->code[1], J_FLOAT);
}

static struct statement *convert_dload(struct compilation_unit *cu,
				       struct basic_block *bb)
{
	return convert_load(cu, bb, cu->code[1], J_DOUBLE);
}

static struct statement *convert_aload(struct compilation_unit *cu,
				       struct basic_block *bb)
{
	return convert_load(cu, bb, cu->code[1], J_REFERENCE);
}

static struct statement *convert_iload_n(struct compilation_unit *cu,
					 struct basic_block *bb)
{
	return convert_load(cu, bb, cu->code[0] - OPC_ILOAD_0, J_INT);
}

static struct statement *convert_lload_n(struct compilation_unit *cu,
					 struct basic_block *bb)
{
	return convert_load(cu, bb, cu->code[0] - OPC_LLOAD_0, J_LONG);
}

static struct statement *convert_fload_n(struct compilation_unit *cu,
					 struct basic_block *bb)
{
	return convert_load(cu, bb, cu->code[0] - OPC_FLOAD_0, J_FLOAT);
}

static struct statement *convert_dload_n(struct compilation_unit *cu,
					 struct basic_block *bb)
{
	return convert_load(cu, bb, cu->code[0] - OPC_DLOAD_0, J_DOUBLE);
}

static struct statement *convert_aload_n(struct compilation_unit *cu,
					 struct basic_block *bb)
{
	return convert_load(cu, bb, cu->code[0] - OPC_ALOAD_0, J_REFERENCE);
}

static struct statement *convert_array_load(struct compilation_unit *cu,
					    struct basic_block *bb,
					    enum jvm_type type)
{
	struct expression *index, *arrayref;
	struct statement *store_stmt, *arraycheck, *nullcheck;

	index = stack_pop(cu->expr_stack);
	arrayref = stack_pop(cu->expr_stack);

	store_stmt = alloc_statement(STMT_STORE);
	if (!store_stmt)
		goto failed;

	store_stmt->store_src = array_deref_expr(type, arrayref, index);
	store_stmt->store_dest = temporary_expr(type, alloc_temporary());

	expr_get(store_stmt->store_dest);
	stack_push(cu->expr_stack, store_stmt->store_dest);

	arraycheck = alloc_statement(STMT_ARRAY_CHECK);
	if (!arraycheck)
		goto failed_arraycheck;

	expr_get(store_stmt->store_src);
	arraycheck->expression = store_stmt->store_src;

	nullcheck = alloc_statement(STMT_NULL_CHECK);
	if (!nullcheck)
		goto failed_nullcheck;

	expr_get(arrayref);
	nullcheck->expression = arrayref;

	bb_insert_stmt(bb, nullcheck);
	bb_insert_stmt(bb, arraycheck);

	return store_stmt;

      failed_nullcheck:
	free_statement(arraycheck);
      failed_arraycheck:
	free_statement(store_stmt);
      failed:
	return NULL;
}

static struct statement *convert_iaload(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return convert_array_load(cu, bb, J_INT);
}

static struct statement *convert_laload(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return convert_array_load(cu, bb, J_LONG);
}

static struct statement *convert_faload(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return convert_array_load(cu, bb, J_FLOAT);
}

static struct statement *convert_daload(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return convert_array_load(cu, bb, J_DOUBLE);
}

static struct statement *convert_aaload(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return convert_array_load(cu, bb, J_REFERENCE);
}

static struct statement *convert_baload(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return convert_array_load(cu, bb, J_INT);
}

static struct statement *convert_caload(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return convert_array_load(cu, bb, J_CHAR);
}

static struct statement *convert_saload(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return convert_array_load(cu, bb, J_SHORT);
}

static struct statement *convert_store(struct compilation_unit *cu,
				       struct basic_block *bb,
				       enum jvm_type type,
				       unsigned long index)
{
	struct statement *stmt = alloc_statement(STMT_STORE);
	if (!stmt)
		goto failed;

	stmt->store_dest = local_expr(type, index);
	stmt->store_src = stack_pop(cu->expr_stack);
	return stmt;
      failed:
	free_statement(stmt);
	return NULL;
}

static struct statement *convert_istore(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return convert_store(cu, bb, J_INT, cu->code[1]);
}

static struct statement *convert_lstore(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return convert_store(cu, bb, J_LONG, cu->code[1]);
}

static struct statement *convert_fstore(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return convert_store(cu, bb, J_FLOAT, cu->code[1]);
}

static struct statement *convert_dstore(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return convert_store(cu, bb, J_DOUBLE, cu->code[1]);
}

static struct statement *convert_astore(struct compilation_unit *cu,
					struct basic_block *bb)
{
	return convert_store(cu, bb, J_REFERENCE, cu->code[1]);
}

static struct statement *convert_istore_n(struct compilation_unit *cu,
					  struct basic_block *bb)
{
	return convert_store(cu, bb, J_INT, cu->code[0] - OPC_ISTORE_0);
}

static struct statement *convert_lstore_n(struct compilation_unit *cu,
					  struct basic_block *bb)
{
	return convert_store(cu, bb, J_LONG, cu->code[0] - OPC_LSTORE_0);
}

static struct statement *convert_fstore_n(struct compilation_unit *cu,
					  struct basic_block *bb)
{
	return convert_store(cu, bb, J_FLOAT,
			       cu->code[0] - OPC_FSTORE_0);
}

static struct statement *convert_dstore_n(struct compilation_unit *cu,
					  struct basic_block *bb)
{
	return convert_store(cu, bb, J_DOUBLE,
			       cu->code[0] - OPC_DSTORE_0);
}

static struct statement *convert_astore_n(struct compilation_unit *cu,
					  struct basic_block *bb)
{
	return convert_store(cu, bb, J_REFERENCE,
			       cu->code[0] - OPC_ASTORE_0);
}

static struct statement *convert_array_store(struct compilation_unit *cu,
					     struct basic_block *bb,
					     enum jvm_type type)
{
	struct expression *value, *index, *arrayref;
	struct statement *store_stmt, *arraycheck, *nullcheck;

	value = stack_pop(cu->expr_stack);
	index = stack_pop(cu->expr_stack);
	arrayref = stack_pop(cu->expr_stack);

	store_stmt = alloc_statement(STMT_STORE);
	if (!store_stmt)
		goto failed;

	store_stmt->store_dest = array_deref_expr(type, arrayref, index);
	store_stmt->store_src = value;

	arraycheck = alloc_statement(STMT_ARRAY_CHECK);
	if (!arraycheck)
		goto failed_arraycheck;

	expr_get(store_stmt->store_dest);
	arraycheck->expression = store_stmt->store_dest;

	nullcheck = alloc_statement(STMT_NULL_CHECK);
	if (!nullcheck)
		goto failed_nullcheck;

	expr_get(arrayref);
	nullcheck->expression = arrayref;

	bb_insert_stmt(bb, nullcheck);
	bb_insert_stmt(bb, arraycheck);

	return store_stmt;

      failed_nullcheck:
	free_statement(arraycheck);
      failed_arraycheck:
	free_statement(store_stmt);
      failed:
	return NULL;
}

static struct statement *convert_iastore(struct compilation_unit *cu,
					 struct basic_block *bb)
{
	return convert_array_store(cu, bb, J_INT);
}

static struct statement *convert_lastore(struct compilation_unit *cu,
					 struct basic_block *bb)
{
	return convert_array_store(cu, bb, J_LONG);
}

static struct statement *convert_fastore(struct compilation_unit *cu,
					 struct basic_block *bb)
{
	return convert_array_store(cu, bb, J_FLOAT);
}

static struct statement *convert_dastore(struct compilation_unit *cu,
					 struct basic_block *bb)
{
	return convert_array_store(cu, bb, J_DOUBLE);
}

static struct statement *convert_aastore(struct compilation_unit *cu,
					 struct basic_block *bb)
{
	return convert_array_store(cu, bb, J_REFERENCE);
}

static struct statement *convert_bastore(struct compilation_unit *cu,
					 struct basic_block *bb)
{
	return convert_array_store(cu, bb, J_INT);
}

static struct statement *convert_castore(struct compilation_unit *cu,
					 struct basic_block *bb)
{
	return convert_array_store(cu, bb, J_CHAR);
}

static struct statement *convert_sastore(struct compilation_unit *cu,
					 struct basic_block *bb)
{
	return convert_array_store(cu, bb, J_SHORT);
}

static struct statement *convert_pop(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	stack_pop(cu->expr_stack);
	return NULL;
}

static struct statement *convert_dup(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	void *value = stack_pop(cu->expr_stack);
	stack_push(cu->expr_stack, value);
	stack_push(cu->expr_stack, value);
	return NULL;
}

static struct statement *convert_dup_x1(struct compilation_unit *cu,
					struct basic_block *bb)
{
	void *value1 = stack_pop(cu->expr_stack);
	void *value2 = stack_pop(cu->expr_stack);
	stack_push(cu->expr_stack, value1);
	stack_push(cu->expr_stack, value2);
	stack_push(cu->expr_stack, value1);
	return NULL;
}

static struct statement *convert_dup_x2(struct compilation_unit *cu,
					struct basic_block *bb)
{
	void *value1 = stack_pop(cu->expr_stack);
	void *value2 = stack_pop(cu->expr_stack);
	void *value3 = stack_pop(cu->expr_stack);
	stack_push(cu->expr_stack, value1);
	stack_push(cu->expr_stack, value3);
	stack_push(cu->expr_stack, value2);
	stack_push(cu->expr_stack, value1);
	return NULL;
}

static struct statement *convert_swap(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	void *value1 = stack_pop(cu->expr_stack);
	void *value2 = stack_pop(cu->expr_stack);
	stack_push(cu->expr_stack, value1);
	stack_push(cu->expr_stack, value2);
	return NULL;
}

static struct statement *convert_binop(struct compilation_unit *cu,
				       struct basic_block *bb,
				       enum jvm_type jvm_type,
				       enum binary_operator binary_operator)
{
	struct expression *left, *right, *expr;

	right = stack_pop(cu->expr_stack);
	left = stack_pop(cu->expr_stack);

	expr = binop_expr(jvm_type, binary_operator, left, right);
	if (expr)
		stack_push(cu->expr_stack, expr);

	return NULL;
}

static struct statement *convert_iadd(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_INT, OP_ADD);
}

static struct statement *convert_ladd(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_LONG, OP_ADD);
}

static struct statement *convert_fadd(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_FLOAT, OP_ADD);
}

static struct statement *convert_dadd(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_DOUBLE, OP_ADD);
}

static struct statement *convert_isub(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_INT, OP_SUB);
}

static struct statement *convert_lsub(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_LONG, OP_SUB);
}

static struct statement *convert_fsub(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_FLOAT, OP_SUB);
}

static struct statement *convert_dsub(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_DOUBLE, OP_SUB);
}

static struct statement *convert_imul(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_INT, OP_MUL);
}

static struct statement *convert_lmul(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_LONG, OP_MUL);
}

static struct statement *convert_fmul(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_FLOAT, OP_MUL);
}

static struct statement *convert_dmul(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_DOUBLE, OP_MUL);
}

static struct statement *convert_idiv(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_INT, OP_DIV);
}

static struct statement *convert_ldiv(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_LONG, OP_DIV);
}

static struct statement *convert_fdiv(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_FLOAT, OP_DIV);
}

static struct statement *convert_ddiv(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_DOUBLE, OP_DIV);
}

static struct statement *convert_irem(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_INT, OP_REM);
}

static struct statement *convert_lrem(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_LONG, OP_REM);
}

static struct statement *convert_frem(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_FLOAT, OP_REM);
}

static struct statement *convert_drem(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_DOUBLE, OP_REM);
}

static struct statement *convert_unary_op(struct compilation_unit *cu,
					  struct basic_block *bb,
					  enum jvm_type jvm_type,
					  enum unary_operator unary_operator)
{
	struct expression *expression, *expr;

	expression = stack_pop(cu->expr_stack);

	expr = unary_op_expr(jvm_type, unary_operator, expression);
	if (expr)
		stack_push(cu->expr_stack, expr);

	return NULL;
}

static struct statement *convert_ineg(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_unary_op(cu, bb, J_INT, OP_NEG);
}

static struct statement *convert_lneg(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_unary_op(cu, bb, J_LONG, OP_NEG);
}

static struct statement *convert_fneg(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_unary_op(cu, bb, J_FLOAT, OP_NEG);
}

static struct statement *convert_dneg(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_unary_op(cu, bb, J_DOUBLE, OP_NEG);
}

static struct statement *convert_ishl(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_INT, OP_SHL);
}

static struct statement *convert_lshl(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_LONG, OP_SHL);
}

static struct statement *convert_ishr(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_INT, OP_SHR);
}

static struct statement *convert_lshr(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_LONG, OP_SHR);
}

static struct statement *convert_iand(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_INT, OP_AND);
}

static struct statement *convert_land(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_LONG, OP_AND);
}

static struct statement *convert_ior(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	return convert_binop(cu, bb, J_INT, OP_OR);
}

static struct statement *convert_lor(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	return convert_binop(cu, bb, J_LONG, OP_OR);
}

static struct statement *convert_ixor(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_INT, OP_XOR);
}

static struct statement *convert_lxor(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_LONG, OP_XOR);
}

static struct statement *convert_iinc(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	struct statement *store_stmt;
	struct expression *local_expression, *binop_expression,
	    *const_expression;

	store_stmt = alloc_statement(STMT_STORE);
	if (!store_stmt)
		goto failed;

	local_expression = local_expr(J_INT, cu->code[1]);
	if (!local_expression)
		goto failed;

	store_stmt->store_dest = local_expression;

	const_expression = value_expr(J_INT, cu->code[2]);
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

	store_stmt->store_src = binop_expression;

	return store_stmt;

      failed:
	free_statement(store_stmt);
	return NULL;
}

static struct statement *convert_conversion(struct compilation_unit *cu,
					    struct basic_block *bb,
					    enum jvm_type to_type)
{
	struct expression *from_expression, *conversion_expression;

	from_expression = stack_pop(cu->expr_stack);

	conversion_expression = conversion_expr(to_type, from_expression);
	if (conversion_expression)
		stack_push(cu->expr_stack, conversion_expression);

	return NULL;
}

static struct statement *convert_i2l(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	return convert_conversion(cu, bb, J_LONG);
}

static struct statement *convert_i2f(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	return convert_conversion(cu, bb, J_FLOAT);
}

static struct statement *convert_i2d(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	return convert_conversion(cu, bb, J_DOUBLE);
}

static struct statement *convert_l2i(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	return convert_conversion(cu, bb, J_INT);
}

static struct statement *convert_l2f(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	return convert_conversion(cu, bb, J_FLOAT);
}

static struct statement *convert_l2d(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	return convert_conversion(cu, bb, J_DOUBLE);
}

static struct statement *convert_f2i(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	return convert_conversion(cu, bb, J_INT);
}

static struct statement *convert_f2l(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	return convert_conversion(cu, bb, J_LONG);
}

static struct statement *convert_f2d(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	return convert_conversion(cu, bb, J_DOUBLE);
}

static struct statement *convert_d2i(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	return convert_conversion(cu, bb, J_INT);
}

static struct statement *convert_d2l(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	return convert_conversion(cu, bb, J_LONG);
}

static struct statement *convert_d2f(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	return convert_conversion(cu, bb, J_FLOAT);
}

static struct statement *convert_i2b(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	return convert_conversion(cu, bb, J_BYTE);
}

static struct statement *convert_i2c(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	return convert_conversion(cu, bb, J_CHAR);
}

static struct statement *convert_i2s(struct compilation_unit *cu,
				     struct basic_block *bb)
{
	return convert_conversion(cu, bb, J_SHORT);
}

static struct statement *convert_lcmp(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_binop(cu, bb, J_INT, OP_CMP);
}

static struct statement *convert_xcmpl(struct compilation_unit *cu,
				       struct basic_block *bb)
{
	return convert_binop(cu, bb, J_INT, OP_CMPL);
}

static struct statement *convert_xcmpg(struct compilation_unit *cu,
				       struct basic_block *bb)
{
	return convert_binop(cu, bb, J_INT, OP_CMPG);
}

static struct statement *__convert_if(struct compilation_unit *cu,
				      struct basic_block *bb,
				      enum jvm_type jvm_type,
				      enum binary_operator binop,
				      struct expression *binary_left,
				      struct expression *binary_right)
{
	struct basic_block *true_bb;
	struct expression *if_conditional;
	struct statement *if_stmt;
	unsigned long if_target;

	if_target = bytecode_br_target(&cu->code[0]);
	true_bb = find_bb(cu, if_target);

	if_conditional = binop_expr(jvm_type, binop, binary_left, binary_right);
	if (!if_conditional)
		goto failed;

	if_stmt = alloc_statement(STMT_IF);
	if (!if_stmt)
		goto failed_put_expr;

	if_stmt->if_true = true_bb->label_stmt;
	if_stmt->if_conditional = if_conditional;

	return if_stmt;
      failed_put_expr:
	expr_put(if_conditional);
      failed:
	return NULL;
}

static struct statement *convert_if(struct compilation_unit *cu,
				    struct basic_block *bb,
				    enum binary_operator binop)
{
	struct statement *stmt;
	struct expression *if_value, *zero_value;

	zero_value = value_expr(J_INT, 0);
	if (!zero_value)
		return NULL;

	if_value = stack_pop(cu->expr_stack);
	stmt = __convert_if(cu, bb, J_INT, binop, if_value, zero_value);
	if (!stmt) {
		expr_put(zero_value);
		return NULL;
	}
	return stmt;
}

static struct statement *convert_ifeq(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_if(cu, bb, OP_EQ);
}

static struct statement *convert_ifne(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_if(cu, bb, OP_NE);
}

static struct statement *convert_iflt(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_if(cu, bb, OP_LT);
}

static struct statement *convert_ifge(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_if(cu, bb, OP_GE);
}

static struct statement *convert_ifgt(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_if(cu, bb, OP_GT);
}

static struct statement *convert_ifle(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	return convert_if(cu, bb, OP_LE);
}

static struct statement *convert_if_cmp(struct compilation_unit *cu,
					struct basic_block *bb,
					enum jvm_type jvm_type,
					enum binary_operator binop)
{
	struct expression *if_value1, *if_value2;

	if_value2 = stack_pop(cu->expr_stack);
	if_value1 = stack_pop(cu->expr_stack);

	return __convert_if(cu, bb, jvm_type, binop, if_value1, if_value2);
}

static struct statement *convert_if_icmpeq(struct compilation_unit *cu,
					   struct basic_block *bb)
{
	return convert_if_cmp(cu, bb, J_INT, OP_EQ);
}

static struct statement *convert_if_icmpne(struct compilation_unit *cu,
					   struct basic_block *bb)
{
	return convert_if_cmp(cu, bb, J_INT, OP_NE);
}

static struct statement *convert_if_icmplt(struct compilation_unit *cu,
					   struct basic_block *bb)
{
	return convert_if_cmp(cu, bb, J_INT, OP_LT);
}

static struct statement *convert_if_icmpge(struct compilation_unit *cu,
					   struct basic_block *bb)
{
	return convert_if_cmp(cu, bb, J_INT, OP_GE);
}

static struct statement *convert_if_icmpgt(struct compilation_unit *cu,
					   struct basic_block *bb)
{
	return convert_if_cmp(cu, bb, J_INT, OP_GT);
}

static struct statement *convert_if_icmple(struct compilation_unit *cu,
					   struct basic_block *bb)
{
	return convert_if_cmp(cu, bb, J_INT, OP_LE);
}

static struct statement *convert_if_acmpeq(struct compilation_unit *cu,
					   struct basic_block *bb)
{
	return convert_if_cmp(cu, bb, J_REFERENCE, OP_EQ);
}

static struct statement *convert_if_acmpne(struct compilation_unit *cu,
					   struct basic_block *bb)
{
	return convert_if_cmp(cu, bb, J_REFERENCE, OP_NE);
}

static struct statement *convert_goto(struct compilation_unit *cu,
				      struct basic_block *bb)
{
	struct basic_block *target_bb;
	struct statement *goto_stmt;
	unsigned long goto_target;

	goto_target = bytecode_br_target(&cu->code[0]);
	target_bb = find_bb(cu, goto_target);

	goto_stmt = alloc_statement(STMT_GOTO);
	if (goto_stmt)
		goto_stmt->goto_target = target_bb->label_stmt;
	return goto_stmt;
}

static struct statement *convert_return(struct compilation_unit *cu,
					struct basic_block *bb)
{
	struct statement *return_stmt = alloc_statement(STMT_RETURN);
	if (return_stmt)
		return_stmt->return_value = stack_pop(cu->expr_stack);
	return return_stmt;
}

typedef struct statement *(*convert_fn_t) (struct compilation_unit *,
					   struct basic_block *);

struct converter {
	convert_fn_t convert;
};

#define DECLARE_CONVERTER(opc, fn) \
		[opc] = { .convert = fn }

static struct converter converters[] = {
	DECLARE_CONVERTER(OPC_NOP, convert_nop),
	DECLARE_CONVERTER(OPC_ACONST_NULL, convert_aconst_null),
	DECLARE_CONVERTER(OPC_ICONST_M1, convert_iconst),
	DECLARE_CONVERTER(OPC_ICONST_0, convert_iconst),
	DECLARE_CONVERTER(OPC_ICONST_1, convert_iconst),
	DECLARE_CONVERTER(OPC_ICONST_2, convert_iconst),
	DECLARE_CONVERTER(OPC_ICONST_3, convert_iconst),
	DECLARE_CONVERTER(OPC_ICONST_4, convert_iconst),
	DECLARE_CONVERTER(OPC_ICONST_5, convert_iconst),
	DECLARE_CONVERTER(OPC_LCONST_0, convert_lconst),
	DECLARE_CONVERTER(OPC_LCONST_1, convert_lconst),
	DECLARE_CONVERTER(OPC_FCONST_0, convert_fconst),
	DECLARE_CONVERTER(OPC_FCONST_1, convert_fconst),
	DECLARE_CONVERTER(OPC_FCONST_2, convert_fconst),
	DECLARE_CONVERTER(OPC_DCONST_0, convert_dconst),
	DECLARE_CONVERTER(OPC_DCONST_1, convert_dconst),
	DECLARE_CONVERTER(OPC_BIPUSH, convert_bipush),
	DECLARE_CONVERTER(OPC_SIPUSH, convert_sipush),
	DECLARE_CONVERTER(OPC_LDC, convert_ldc),
	DECLARE_CONVERTER(OPC_LDC_W, convert_ldc_w),
	DECLARE_CONVERTER(OPC_LDC2_W, convert_ldc2_w),
	DECLARE_CONVERTER(OPC_ILOAD, convert_iload),
	DECLARE_CONVERTER(OPC_LLOAD, convert_lload),
	DECLARE_CONVERTER(OPC_FLOAD, convert_fload),
	DECLARE_CONVERTER(OPC_DLOAD, convert_dload),
	DECLARE_CONVERTER(OPC_ALOAD, convert_aload),
	DECLARE_CONVERTER(OPC_ILOAD_0, convert_iload_n),
	DECLARE_CONVERTER(OPC_ILOAD_1, convert_iload_n),
	DECLARE_CONVERTER(OPC_ILOAD_2, convert_iload_n),
	DECLARE_CONVERTER(OPC_ILOAD_3, convert_iload_n),
	DECLARE_CONVERTER(OPC_LLOAD_0, convert_lload_n),
	DECLARE_CONVERTER(OPC_LLOAD_1, convert_lload_n),
	DECLARE_CONVERTER(OPC_LLOAD_2, convert_lload_n),
	DECLARE_CONVERTER(OPC_LLOAD_3, convert_lload_n),
	DECLARE_CONVERTER(OPC_FLOAD_0, convert_fload_n),
	DECLARE_CONVERTER(OPC_FLOAD_1, convert_fload_n),
	DECLARE_CONVERTER(OPC_FLOAD_2, convert_fload_n),
	DECLARE_CONVERTER(OPC_FLOAD_3, convert_fload_n),
	DECLARE_CONVERTER(OPC_DLOAD_0, convert_dload_n),
	DECLARE_CONVERTER(OPC_DLOAD_1, convert_dload_n),
	DECLARE_CONVERTER(OPC_DLOAD_2, convert_dload_n),
	DECLARE_CONVERTER(OPC_DLOAD_3, convert_dload_n),
	DECLARE_CONVERTER(OPC_ALOAD_0, convert_aload_n),
	DECLARE_CONVERTER(OPC_ALOAD_1, convert_aload_n),
	DECLARE_CONVERTER(OPC_ALOAD_2, convert_aload_n),
	DECLARE_CONVERTER(OPC_ALOAD_3, convert_aload_n),
	DECLARE_CONVERTER(OPC_IALOAD, convert_iaload),
	DECLARE_CONVERTER(OPC_LALOAD, convert_laload),
	DECLARE_CONVERTER(OPC_FALOAD, convert_faload),
	DECLARE_CONVERTER(OPC_DALOAD, convert_daload),
	DECLARE_CONVERTER(OPC_AALOAD, convert_aaload),
	DECLARE_CONVERTER(OPC_BALOAD, convert_baload),
	DECLARE_CONVERTER(OPC_CALOAD, convert_caload),
	DECLARE_CONVERTER(OPC_SALOAD, convert_saload),
	DECLARE_CONVERTER(OPC_ISTORE, convert_istore),
	DECLARE_CONVERTER(OPC_LSTORE, convert_lstore),
	DECLARE_CONVERTER(OPC_FSTORE, convert_fstore),
	DECLARE_CONVERTER(OPC_DSTORE, convert_dstore),
	DECLARE_CONVERTER(OPC_ASTORE, convert_astore),
	DECLARE_CONVERTER(OPC_ISTORE_0, convert_istore_n),
	DECLARE_CONVERTER(OPC_ISTORE_1, convert_istore_n),
	DECLARE_CONVERTER(OPC_ISTORE_2, convert_istore_n),
	DECLARE_CONVERTER(OPC_ISTORE_3, convert_istore_n),
	DECLARE_CONVERTER(OPC_LSTORE_0, convert_lstore_n),
	DECLARE_CONVERTER(OPC_LSTORE_1, convert_lstore_n),
	DECLARE_CONVERTER(OPC_LSTORE_2, convert_lstore_n),
	DECLARE_CONVERTER(OPC_LSTORE_3, convert_lstore_n),
	DECLARE_CONVERTER(OPC_FSTORE_0, convert_fstore_n),
	DECLARE_CONVERTER(OPC_FSTORE_1, convert_fstore_n),
	DECLARE_CONVERTER(OPC_FSTORE_2, convert_fstore_n),
	DECLARE_CONVERTER(OPC_FSTORE_3, convert_fstore_n),
	DECLARE_CONVERTER(OPC_DSTORE_0, convert_dstore_n),
	DECLARE_CONVERTER(OPC_DSTORE_1, convert_dstore_n),
	DECLARE_CONVERTER(OPC_DSTORE_2, convert_dstore_n),
	DECLARE_CONVERTER(OPC_DSTORE_3, convert_dstore_n),
	DECLARE_CONVERTER(OPC_ASTORE_0, convert_astore_n),
	DECLARE_CONVERTER(OPC_ASTORE_1, convert_astore_n),
	DECLARE_CONVERTER(OPC_ASTORE_2, convert_astore_n),
	DECLARE_CONVERTER(OPC_ASTORE_3, convert_astore_n),
	DECLARE_CONVERTER(OPC_IASTORE, convert_iastore),
	DECLARE_CONVERTER(OPC_LASTORE, convert_lastore),
	DECLARE_CONVERTER(OPC_FASTORE, convert_fastore),
	DECLARE_CONVERTER(OPC_DASTORE, convert_dastore),
	DECLARE_CONVERTER(OPC_AASTORE, convert_aastore),
	DECLARE_CONVERTER(OPC_BASTORE, convert_bastore),
	DECLARE_CONVERTER(OPC_CASTORE, convert_castore),
	DECLARE_CONVERTER(OPC_SASTORE, convert_sastore),
	DECLARE_CONVERTER(OPC_POP, convert_pop),
	DECLARE_CONVERTER(OPC_POP2, convert_pop),
	DECLARE_CONVERTER(OPC_DUP, convert_dup),
	DECLARE_CONVERTER(OPC_DUP_X1, convert_dup_x1),
	DECLARE_CONVERTER(OPC_DUP_X2, convert_dup_x2),
	DECLARE_CONVERTER(OPC_DUP2, convert_dup),
	DECLARE_CONVERTER(OPC_DUP2_X1, convert_dup_x1),
	DECLARE_CONVERTER(OPC_DUP2_X2, convert_dup_x2),
	DECLARE_CONVERTER(OPC_SWAP, convert_swap),
	DECLARE_CONVERTER(OPC_IADD, convert_iadd),
	DECLARE_CONVERTER(OPC_LADD, convert_ladd),
	DECLARE_CONVERTER(OPC_FADD, convert_fadd),
	DECLARE_CONVERTER(OPC_DADD, convert_dadd),
	DECLARE_CONVERTER(OPC_ISUB, convert_isub),
	DECLARE_CONVERTER(OPC_LSUB, convert_lsub),
	DECLARE_CONVERTER(OPC_FSUB, convert_fsub),
	DECLARE_CONVERTER(OPC_DSUB, convert_dsub),
	DECLARE_CONVERTER(OPC_IMUL, convert_imul),
	DECLARE_CONVERTER(OPC_LMUL, convert_lmul),
	DECLARE_CONVERTER(OPC_FMUL, convert_fmul),
	DECLARE_CONVERTER(OPC_DMUL, convert_dmul),
	DECLARE_CONVERTER(OPC_IDIV, convert_idiv),
	DECLARE_CONVERTER(OPC_LDIV, convert_ldiv),
	DECLARE_CONVERTER(OPC_FDIV, convert_fdiv),
	DECLARE_CONVERTER(OPC_DDIV, convert_ddiv),
	DECLARE_CONVERTER(OPC_IREM, convert_irem),
	DECLARE_CONVERTER(OPC_LREM, convert_lrem),
	DECLARE_CONVERTER(OPC_FREM, convert_frem),
	DECLARE_CONVERTER(OPC_DREM, convert_drem),
	DECLARE_CONVERTER(OPC_INEG, convert_ineg),
	DECLARE_CONVERTER(OPC_LNEG, convert_lneg),
	DECLARE_CONVERTER(OPC_FNEG, convert_fneg),
	DECLARE_CONVERTER(OPC_DNEG, convert_dneg),
	DECLARE_CONVERTER(OPC_ISHL, convert_ishl),
	DECLARE_CONVERTER(OPC_LSHL, convert_lshl),
	DECLARE_CONVERTER(OPC_ISHR, convert_ishr),
	DECLARE_CONVERTER(OPC_LSHR, convert_lshr),
	DECLARE_CONVERTER(OPC_IAND, convert_iand),
	DECLARE_CONVERTER(OPC_LAND, convert_land),
	DECLARE_CONVERTER(OPC_IOR, convert_ior),
	DECLARE_CONVERTER(OPC_LOR, convert_lor),
	DECLARE_CONVERTER(OPC_IXOR, convert_ixor),
	DECLARE_CONVERTER(OPC_LXOR, convert_lxor),
	DECLARE_CONVERTER(OPC_IINC, convert_iinc),
	DECLARE_CONVERTER(OPC_I2L, convert_i2l),
	DECLARE_CONVERTER(OPC_I2F, convert_i2f),
	DECLARE_CONVERTER(OPC_I2D, convert_i2d),
	DECLARE_CONVERTER(OPC_L2I, convert_l2i),
	DECLARE_CONVERTER(OPC_L2F, convert_l2f),
	DECLARE_CONVERTER(OPC_L2D, convert_l2d),
	DECLARE_CONVERTER(OPC_F2I, convert_f2i),
	DECLARE_CONVERTER(OPC_F2L, convert_f2l),
	DECLARE_CONVERTER(OPC_F2D, convert_f2d),
	DECLARE_CONVERTER(OPC_D2I, convert_d2i),
	DECLARE_CONVERTER(OPC_D2L, convert_d2l),
	DECLARE_CONVERTER(OPC_D2F, convert_d2f),
	DECLARE_CONVERTER(OPC_I2B, convert_i2b),
	DECLARE_CONVERTER(OPC_I2C, convert_i2c),
	DECLARE_CONVERTER(OPC_I2S, convert_i2s),
	DECLARE_CONVERTER(OPC_LCMP, convert_lcmp),
	DECLARE_CONVERTER(OPC_FCMPL, convert_xcmpl),
	DECLARE_CONVERTER(OPC_FCMPG, convert_xcmpg),
	DECLARE_CONVERTER(OPC_DCMPL, convert_xcmpl),
	DECLARE_CONVERTER(OPC_DCMPG, convert_xcmpg),
	DECLARE_CONVERTER(OPC_IFEQ, convert_ifeq),
	DECLARE_CONVERTER(OPC_IFNE, convert_ifne),
	DECLARE_CONVERTER(OPC_IFLT, convert_iflt),
	DECLARE_CONVERTER(OPC_IFGE, convert_ifge),
	DECLARE_CONVERTER(OPC_IFGT, convert_ifgt),
	DECLARE_CONVERTER(OPC_IFLE, convert_ifle),
	DECLARE_CONVERTER(OPC_IF_ICMPEQ, convert_if_icmpeq),
	DECLARE_CONVERTER(OPC_IF_ICMPNE, convert_if_icmpne),
	DECLARE_CONVERTER(OPC_IF_ICMPLT, convert_if_icmplt),
	DECLARE_CONVERTER(OPC_IF_ICMPGE, convert_if_icmpge),
	DECLARE_CONVERTER(OPC_IF_ICMPGT, convert_if_icmpgt),
	DECLARE_CONVERTER(OPC_IF_ICMPLE, convert_if_icmple),
	DECLARE_CONVERTER(OPC_IF_ACMPEQ, convert_if_acmpeq),
	DECLARE_CONVERTER(OPC_IF_ACMPNE, convert_if_acmpne),
	DECLARE_CONVERTER(OPC_GOTO, convert_goto),
	DECLARE_CONVERTER(OPC_IRETURN, convert_return),
	DECLARE_CONVERTER(OPC_LRETURN, convert_return),
	DECLARE_CONVERTER(OPC_FRETURN, convert_return),
	DECLARE_CONVERTER(OPC_DRETURN, convert_return),
	DECLARE_CONVERTER(OPC_ARETURN, convert_return),
};

/**
 *	convert_to_ir - Convert bytecode to intermediate representation.
 *	@compilation_unit: compilation unit to convert.
 *
 *	This function converts bytecode in a compilation unit to intermediate
 *	representation of the JIT compiler.
 *
 *	Returns 1 if conversion succeeded; 0 otherwise.
 */
int convert_to_ir(struct compilation_unit *cu)
{
	struct statement *stmt;
	unsigned char opc = cu->code[0];
	struct converter *converter = &converters[opc];

	if (!cu->entry_bb || !converter
	    || cu->code_len < bytecode_size(cu->code))
		return 0;

	stmt = converter->convert(cu, cu->entry_bb);
	if (stmt)
		bb_insert_stmt(cu->entry_bb, stmt);
	return 1;
}
