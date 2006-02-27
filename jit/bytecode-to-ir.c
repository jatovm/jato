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
#include <errno.h>

#include <stdlib.h>

static unsigned long alloc_temporary(void)
{
	static unsigned long temporary;
	return ++temporary;
}

static int convert_nop(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	struct statement *stmt;

	stmt = alloc_statement(STMT_NOP);
	if (!stmt)
		return -ENOMEM;

	bb_insert_stmt(bb, stmt);
	return 0;
}

static int __convert_const(enum jvm_type jvm_type,
			   unsigned long long value, struct stack *expr_stack)
{
	struct expression *expr = value_expr(jvm_type, value);
	if (!expr)
		return -ENOMEM;

	stack_push(expr_stack, expr);
	return 0;
}

static int convert_aconst_null(struct compilation_unit *cu,
			       struct basic_block *bb, unsigned long offset)
{
	return __convert_const(J_REFERENCE, 0, cu->expr_stack);
}

static int convert_iconst(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return __convert_const(J_INT, cu->code[offset] - OPC_ICONST_0,
			       cu->expr_stack);
}

static int convert_lconst(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return __convert_const(J_LONG, cu->code[offset] - OPC_LCONST_0,
			       cu->expr_stack);
}

static int __convert_fconst(enum jvm_type jvm_type,
			    double value, struct stack *expr_stack)
{
	struct expression *expr = fvalue_expr(jvm_type, value);
	if (!expr)
		return -ENOMEM;

	stack_push(expr_stack, expr);
	return 0;
}

static int convert_fconst(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return __convert_fconst(J_FLOAT,
				cu->code[offset] - OPC_FCONST_0,
				cu->expr_stack);
}

static int convert_dconst(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return __convert_fconst(J_DOUBLE,
				cu->code[offset] - OPC_DCONST_0,
				cu->expr_stack);
}

static int convert_bipush(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return __convert_const(J_INT, (char)cu->code[offset + 1],
			       cu->expr_stack);
}

static int convert_sipush(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return __convert_const(J_INT,
			       (short)be16_to_cpu(*(u2 *) & cu->
						  code[offset + 1]),
			       cu->expr_stack);
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
		expr = value_expr(J_REFERENCE, CP_LONG(cp, cp_idx));
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

static int convert_ldc(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	return __convert_ldc(&cu->cb->constant_pool,
			     cu->code[offset + 1], cu->expr_stack);
}

static int convert_ldc_w(struct compilation_unit *cu, struct basic_block *bb,
			 unsigned long offset)
{
	return __convert_ldc(&cu->cb->constant_pool,
			     be16_to_cpu(*(u2 *) & cu->code[offset + 1]),
			     cu->expr_stack);
}

static int convert_ldc2_w(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return __convert_ldc(&cu->cb->constant_pool,
			     be16_to_cpu(*(u2 *) & cu->code[offset + 1]),
			     cu->expr_stack);
}

static int convert_load(struct compilation_unit *cu,
			struct basic_block *bb,
			unsigned char index, enum jvm_type type)
{
	struct expression *expr;

	expr = local_expr(type, index);
	if (!expr)
		return -ENOMEM;

	stack_push(cu->expr_stack, expr);
	return 0;
}

static int convert_iload(struct compilation_unit *cu, struct basic_block *bb,
			 unsigned long offset)
{
	return convert_load(cu, bb, cu->code[offset + 1], J_INT);
}

static int convert_lload(struct compilation_unit *cu, struct basic_block *bb,
			 unsigned long offset)
{
	return convert_load(cu, bb, cu->code[offset + 1], J_LONG);
}

static int convert_fload(struct compilation_unit *cu, struct basic_block *bb,
			 unsigned long offset)
{
	return convert_load(cu, bb, cu->code[offset + 1], J_FLOAT);
}

static int convert_dload(struct compilation_unit *cu, struct basic_block *bb,
			 unsigned long offset)
{
	return convert_load(cu, bb, cu->code[offset + 1], J_DOUBLE);
}

static int convert_aload(struct compilation_unit *cu, struct basic_block *bb,
			 unsigned long offset)
{
	return convert_load(cu, bb, cu->code[offset + 1], J_REFERENCE);
}

static int convert_iload_n(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_load(cu, bb, cu->code[offset] - OPC_ILOAD_0, J_INT);
}

static int convert_lload_n(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_load(cu, bb, cu->code[offset] - OPC_LLOAD_0, J_LONG);
}

static int convert_fload_n(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_load(cu, bb, cu->code[offset] - OPC_FLOAD_0, J_FLOAT);
}

static int convert_dload_n(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_load(cu, bb, cu->code[offset] - OPC_DLOAD_0, J_DOUBLE);
}

static int convert_aload_n(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_load(cu, bb, cu->code[offset] - OPC_ALOAD_0,
			    J_REFERENCE);
}

static int convert_array_load(struct compilation_unit *cu,
			      struct basic_block *bb, enum jvm_type type)
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
	bb_insert_stmt(bb, store_stmt);

	return 0;

      failed_nullcheck:
	free_statement(arraycheck);
      failed_arraycheck:
	free_statement(store_stmt);
      failed:
	return -ENOMEM;
}

static int convert_iaload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_INT);
}

static int convert_laload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_LONG);
}

static int convert_faload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_FLOAT);
}

static int convert_daload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_DOUBLE);
}

static int convert_aaload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_REFERENCE);
}

static int convert_baload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_INT);
}

static int convert_caload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_CHAR);
}

static int convert_saload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_SHORT);
}

static int convert_store(struct compilation_unit *cu,
			 struct basic_block *bb,
			 enum jvm_type type, unsigned long index)
{
	struct statement *stmt = alloc_statement(STMT_STORE);
	if (!stmt)
		goto failed;

	stmt->store_dest = local_expr(type, index);
	stmt->store_src = stack_pop(cu->expr_stack);
	bb_insert_stmt(bb, stmt);
	return 0;
      failed:
	free_statement(stmt);
	return -ENOMEM;
}

static int convert_istore(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_store(cu, bb, J_INT, cu->code[offset + 1]);
}

static int convert_lstore(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_store(cu, bb, J_LONG, cu->code[offset + 1]);
}

static int convert_fstore(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_store(cu, bb, J_FLOAT, cu->code[offset + 1]);
}

static int convert_dstore(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_store(cu, bb, J_DOUBLE, cu->code[offset + 1]);
}

static int convert_astore(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_store(cu, bb, J_REFERENCE, cu->code[offset + 1]);
}

static int convert_istore_n(struct compilation_unit *cu, struct basic_block *bb,
			    unsigned long offset)
{
	return convert_store(cu, bb, J_INT, cu->code[offset] - OPC_ISTORE_0);
}

static int convert_lstore_n(struct compilation_unit *cu, struct basic_block *bb,
			    unsigned long offset)
{
	return convert_store(cu, bb, J_LONG, cu->code[offset] - OPC_LSTORE_0);
}

static int convert_fstore_n(struct compilation_unit *cu, struct basic_block *bb,
			    unsigned long offset)
{
	return convert_store(cu, bb, J_FLOAT, cu->code[offset] - OPC_FSTORE_0);
}

static int convert_dstore_n(struct compilation_unit *cu, struct basic_block *bb,
			    unsigned long offset)
{
	return convert_store(cu, bb, J_DOUBLE, cu->code[offset] - OPC_DSTORE_0);
}

static int convert_astore_n(struct compilation_unit *cu, struct basic_block *bb,
			    unsigned long offset)
{
	return convert_store(cu, bb, J_REFERENCE,
			     cu->code[offset] - OPC_ASTORE_0);
}

static int convert_array_store(struct compilation_unit *cu,
			       struct basic_block *bb, enum jvm_type type)
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
	bb_insert_stmt(bb, store_stmt);

	return 0;

      failed_nullcheck:
	free_statement(arraycheck);
      failed_arraycheck:
	free_statement(store_stmt);
      failed:
	return -ENOMEM;
}

static int convert_iastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_INT);
}

static int convert_lastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_LONG);
}

static int convert_fastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_FLOAT);
}

static int convert_dastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_DOUBLE);
}

static int convert_aastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_REFERENCE);
}

static int convert_bastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_INT);
}

static int convert_castore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_CHAR);
}

static int convert_sastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_SHORT);
}

static int convert_pop(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	stack_pop(cu->expr_stack);
	return 0;
}

static int convert_dup(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	void *value;

	value = stack_pop(cu->expr_stack);
	stack_push(cu->expr_stack, value);
	stack_push(cu->expr_stack, value);
	return 0;
}

static int convert_dup_x1(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	void *value1, *value2;

	value1 = stack_pop(cu->expr_stack);
	value2 = stack_pop(cu->expr_stack);

	stack_push(cu->expr_stack, value1);
	stack_push(cu->expr_stack, value2);
	stack_push(cu->expr_stack, value1);
	return 0;
}

static int convert_dup_x2(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	void *value1, *value2, *value3;

	value1 = stack_pop(cu->expr_stack);
	value2 = stack_pop(cu->expr_stack);
	value3 = stack_pop(cu->expr_stack);

	stack_push(cu->expr_stack, value1);
	stack_push(cu->expr_stack, value3);
	stack_push(cu->expr_stack, value2);
	stack_push(cu->expr_stack, value1);
	return 0;
}

static int convert_swap(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	void *value1, *value2;

	value1 = stack_pop(cu->expr_stack);
	value2 = stack_pop(cu->expr_stack);

	stack_push(cu->expr_stack, value1);
	stack_push(cu->expr_stack, value2);
	return 0;
}

static int convert_binop(struct compilation_unit *cu,
			 struct basic_block *bb,
			 enum jvm_type jvm_type,
			 enum binary_operator binary_operator)
{
	struct expression *left, *right, *expr;

	right = stack_pop(cu->expr_stack);
	left = stack_pop(cu->expr_stack);

	expr = binop_expr(jvm_type, binary_operator, left, right);
	if (!expr)
		return -ENOMEM;

	stack_push(cu->expr_stack, expr);
	return 0;
}

static int convert_iadd(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_ADD);
}

static int convert_ladd(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_ADD);
}

static int convert_fadd(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_FLOAT, OP_ADD);
}

static int convert_dadd(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_DOUBLE, OP_ADD);
}

static int convert_isub(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_SUB);
}

static int convert_lsub(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_SUB);
}

static int convert_fsub(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_FLOAT, OP_SUB);
}

static int convert_dsub(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_DOUBLE, OP_SUB);
}

static int convert_imul(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_MUL);
}

static int convert_lmul(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_MUL);
}

static int convert_fmul(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_FLOAT, OP_MUL);
}

static int convert_dmul(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_DOUBLE, OP_MUL);
}

static int convert_idiv(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_DIV);
}

static int convert_ldiv(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_DIV);
}

static int convert_fdiv(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_FLOAT, OP_DIV);
}

static int convert_ddiv(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_DOUBLE, OP_DIV);
}

static int convert_irem(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_REM);
}

static int convert_lrem(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_REM);
}

static int convert_frem(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_FLOAT, OP_REM);
}

static int convert_drem(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_DOUBLE, OP_REM);
}

static int convert_unary_op(struct compilation_unit *cu,
			    struct basic_block *bb,
			    enum jvm_type jvm_type,
			    enum unary_operator unary_operator)
{
	struct expression *expression, *expr;

	expression = stack_pop(cu->expr_stack);

	expr = unary_op_expr(jvm_type, unary_operator, expression);
	if (!expr)
		return -ENOMEM;

	stack_push(cu->expr_stack, expr);
	return 0;
}

static int convert_ineg(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_unary_op(cu, bb, J_INT, OP_NEG);
}

static int convert_lneg(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_unary_op(cu, bb, J_LONG, OP_NEG);
}

static int convert_fneg(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_unary_op(cu, bb, J_FLOAT, OP_NEG);
}

static int convert_dneg(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_unary_op(cu, bb, J_DOUBLE, OP_NEG);
}

static int convert_ishl(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_SHL);
}

static int convert_lshl(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_SHL);
}

static int convert_ishr(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_SHR);
}

static int convert_lshr(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_SHR);
}

static int convert_iand(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_AND);
}

static int convert_land(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_AND);
}

static int convert_ior(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_OR);
}

static int convert_lor(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_OR);
}

static int convert_ixor(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_XOR);
}

static int convert_lxor(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_LONG, OP_XOR);
}

static int convert_iinc(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	struct statement *store_stmt;
	struct expression *local_expression, *binop_expression,
	    *const_expression;

	store_stmt = alloc_statement(STMT_STORE);
	if (!store_stmt)
		goto failed;

	local_expression = local_expr(J_INT, cu->code[offset + 1]);
	if (!local_expression)
		goto failed;

	store_stmt->store_dest = local_expression;

	const_expression = value_expr(J_INT, cu->code[offset + 2]);
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
	bb_insert_stmt(bb, store_stmt);

	return 0;

      failed:
	free_statement(store_stmt);
	return -ENOMEM;
}

static int convert_conversion(struct compilation_unit *cu,
			      struct basic_block *bb, enum jvm_type to_type)
{
	struct expression *from_expression, *conversion_expression;

	from_expression = stack_pop(cu->expr_stack);

	conversion_expression = conversion_expr(to_type, from_expression);
	if (!conversion_expression)
		return -ENOMEM;

	stack_push(cu->expr_stack, conversion_expression);
	return 0;
}

static int convert_i2l(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	return convert_conversion(cu, bb, J_LONG);
}

static int convert_i2f(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	return convert_conversion(cu, bb, J_FLOAT);
}

static int convert_i2d(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	return convert_conversion(cu, bb, J_DOUBLE);
}

static int convert_l2i(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	return convert_conversion(cu, bb, J_INT);
}

static int convert_l2f(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	return convert_conversion(cu, bb, J_FLOAT);
}

static int convert_l2d(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	return convert_conversion(cu, bb, J_DOUBLE);
}

static int convert_f2i(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	return convert_conversion(cu, bb, J_INT);
}

static int convert_f2l(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	return convert_conversion(cu, bb, J_LONG);
}

static int convert_f2d(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	return convert_conversion(cu, bb, J_DOUBLE);
}

static int convert_d2i(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	return convert_conversion(cu, bb, J_INT);
}

static int convert_d2l(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	return convert_conversion(cu, bb, J_LONG);
}

static int convert_d2f(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	return convert_conversion(cu, bb, J_FLOAT);
}

static int convert_i2b(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	return convert_conversion(cu, bb, J_BYTE);
}

static int convert_i2c(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	return convert_conversion(cu, bb, J_CHAR);
}

static int convert_i2s(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	return convert_conversion(cu, bb, J_SHORT);
}

static int convert_lcmp(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_CMP);
}

static int convert_xcmpl(struct compilation_unit *cu, struct basic_block *bb,
			 unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_CMPL);
}

static int convert_xcmpg(struct compilation_unit *cu, struct basic_block *bb,
			 unsigned long offset)
{
	return convert_binop(cu, bb, J_INT, OP_CMPG);
}

static struct statement *__convert_if(struct compilation_unit *cu,
				      struct basic_block *bb,
				      unsigned long offset,
				      enum jvm_type jvm_type,
				      enum binary_operator binop,
				      struct expression *binary_left,
				      struct expression *binary_right)
{
	struct basic_block *true_bb;
	struct expression *if_conditional;
	struct statement *if_stmt;
	unsigned long if_target;

	if_target = bytecode_br_target(&cu->code[offset]);
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

static int convert_if(struct compilation_unit *cu,
		      struct basic_block *bb, unsigned long offset,
		      enum binary_operator binop)
{
	struct statement *stmt;
	struct expression *if_value, *zero_value;

	zero_value = value_expr(J_INT, 0);
	if (!zero_value)
		return -ENOMEM;

	if_value = stack_pop(cu->expr_stack);
	stmt = __convert_if(cu, bb, offset, J_INT, binop, if_value, zero_value);
	if (!stmt) {
		expr_put(zero_value);
		return -ENOMEM;
	}
	bb_insert_stmt(bb, stmt);
	return 0;
}

static int convert_ifeq(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_if(cu, bb, offset, OP_EQ);
}

static int convert_ifne(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_if(cu, bb, offset, OP_NE);
}

static int convert_iflt(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_if(cu, bb, offset, OP_LT);
}

static int convert_ifge(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_if(cu, bb, offset, OP_GE);
}

static int convert_ifgt(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_if(cu, bb, offset, OP_GT);
}

static int convert_ifle(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	return convert_if(cu, bb, offset, OP_LE);
}

static int convert_if_cmp(struct compilation_unit *cu,
			  struct basic_block *bb,
			  unsigned long offset,
			  enum jvm_type jvm_type, enum binary_operator binop)
{
	struct statement *stmt;
	struct expression *if_value1, *if_value2;

	if_value2 = stack_pop(cu->expr_stack);
	if_value1 = stack_pop(cu->expr_stack);

	stmt =
	    __convert_if(cu, bb, offset, jvm_type, binop, if_value1, if_value2);
	if (!stmt)
		return -ENOMEM;

	bb_insert_stmt(bb, stmt);
	return 0;
}

static int convert_if_icmpeq(struct compilation_unit *cu,
			     struct basic_block *bb, unsigned long offset)
{
	return convert_if_cmp(cu, bb, offset, J_INT, OP_EQ);
}

static int convert_if_icmpne(struct compilation_unit *cu,
			     struct basic_block *bb, unsigned long offset)
{
	return convert_if_cmp(cu, bb, offset, J_INT, OP_NE);
}

static int convert_if_icmplt(struct compilation_unit *cu,
			     struct basic_block *bb, unsigned long offset)
{
	return convert_if_cmp(cu, bb, offset, J_INT, OP_LT);
}

static int convert_if_icmpge(struct compilation_unit *cu,
			     struct basic_block *bb, unsigned long offset)
{
	return convert_if_cmp(cu, bb, offset, J_INT, OP_GE);
}

static int convert_if_icmpgt(struct compilation_unit *cu,
			     struct basic_block *bb, unsigned long offset)
{
	return convert_if_cmp(cu, bb, offset, J_INT, OP_GT);
}

static int convert_if_icmple(struct compilation_unit *cu,
			     struct basic_block *bb, unsigned long offset)
{
	return convert_if_cmp(cu, bb, offset, J_INT, OP_LE);
}

static int convert_if_acmpeq(struct compilation_unit *cu,
			     struct basic_block *bb, unsigned long offset)
{
	return convert_if_cmp(cu, bb, offset, J_REFERENCE, OP_EQ);
}

static int convert_if_acmpne(struct compilation_unit *cu,
			     struct basic_block *bb, unsigned long offset)
{
	return convert_if_cmp(cu, bb, offset, J_REFERENCE, OP_NE);
}

static int convert_goto(struct compilation_unit *cu, struct basic_block *bb,
			unsigned long offset)
{
	struct basic_block *target_bb;
	struct statement *goto_stmt;
	unsigned long goto_target;

	goto_target = bytecode_br_target(&cu->code[offset]);
	target_bb = find_bb(cu, goto_target);

	goto_stmt = alloc_statement(STMT_GOTO);
	if (!goto_stmt)
		return -ENOMEM;

	goto_stmt->goto_target = target_bb->label_stmt;
	bb_insert_stmt(bb, goto_stmt);
	return 0;
}

static int convert_non_void_return(struct compilation_unit *cu,
				   struct basic_block *bb,
				   unsigned long offset)
{
	struct statement *return_stmt = alloc_statement(STMT_RETURN);
	if (!return_stmt)
		return -ENOMEM;

	return_stmt->return_value = stack_pop(cu->expr_stack);
	bb_insert_stmt(bb, return_stmt);
	return 0;
}

static int convert_void_return(struct compilation_unit *cu,
			       struct basic_block *bb,
			       unsigned long offset)
{
	struct statement *return_stmt = alloc_statement(STMT_RETURN);
	if (!return_stmt)
		return -ENOMEM;

	return_stmt->return_value = NULL;
	bb_insert_stmt(bb, return_stmt);
	return 0;
}

static int convert_getstatic(struct compilation_unit *cu,
			     struct basic_block *bb,
			     unsigned long offset)
{
	struct constant_pool *cp;
	unsigned short index;
	struct expression *value;
	u1 type;

	cp = &cu->cb->constant_pool;
	index = be16_to_cpu(*(u2 *) & cu->code[offset + 1]);
	type = CP_TYPE(cp, index);
	
	if (type != CONSTANT_Resolved)
		return -EINVAL;
	
	value = field_expr(J_REFERENCE, (struct fieldblock *) CP_INFO(cp, index));
	if (!value)
		return -ENOMEM;

	stack_push(cu->expr_stack, value);
	return 0;
}

typedef int (*convert_fn_t) (struct compilation_unit *, struct basic_block *,
			     unsigned long);

#define DECLARE_CONVERTER(opc, fn) [opc] = fn

static convert_fn_t converters[] = {
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
	DECLARE_CONVERTER(OPC_IRETURN, convert_non_void_return),
	DECLARE_CONVERTER(OPC_LRETURN, convert_non_void_return),
	DECLARE_CONVERTER(OPC_FRETURN, convert_non_void_return),
	DECLARE_CONVERTER(OPC_DRETURN, convert_non_void_return),
	DECLARE_CONVERTER(OPC_ARETURN, convert_non_void_return),
	DECLARE_CONVERTER(OPC_RETURN, convert_void_return),
	DECLARE_CONVERTER(OPC_GETSTATIC, convert_getstatic),
};

/**
 *	convert_to_ir - Convert bytecode to intermediate representation.
 *	@compilation_unit: compilation unit to convert.
 *
 *	This function converts bytecode in a compilation unit to intermediate
 *	representation of the JIT compiler.
 *
 *	Returns zero if conversion succeeded; otherwise returns a negative
 * 	integer.
 */
int convert_to_ir(struct compilation_unit *cu)
{
	int err = 0;
	unsigned long offset = 0;
	
	if (!cu->entry_bb)
		return -EINVAL;

	while (offset < cu->code_len) {
		unsigned char opc = cu->code[offset];
		convert_fn_t convert = converters[opc];
		unsigned long opc_size;
		
		opc_size = bytecode_size(cu->code);

		if (!convert || cu->code_len-offset < opc_size) {
			err = -EINVAL;
			goto out;
		}

		err = convert(cu, cu->entry_bb, offset);
		if (err)
			goto out;

		offset += opc_size;
	}
  out:
	return err;
}
