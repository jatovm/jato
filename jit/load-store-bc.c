/*
 * Copyright (C) 2005-2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 *
 * The file contains functions for converting Java bytecode load and store
 * instructions to immediate representation of the JIT compiler.
 */

#include <statement.h>
#include <byteorder.h>
#include <stack.h>
#include <jit-compiler.h>
#include <bytecodes.h>
#include <errno.h>

static unsigned long alloc_temporary(void)
{
	static unsigned long temporary;
	return ++temporary;
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

int convert_aconst_null(struct compilation_unit *cu,
			       struct basic_block *bb, unsigned long offset)
{
	return __convert_const(J_REFERENCE, 0, cu->expr_stack);
}

int convert_iconst(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return __convert_const(J_INT, code[offset] - OPC_ICONST_0,
			       cu->expr_stack);
}

int convert_lconst(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return __convert_const(J_LONG, code[offset] - OPC_LCONST_0,
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

int convert_fconst(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return __convert_fconst(J_FLOAT,
				code[offset] - OPC_FCONST_0,
				cu->expr_stack);
}

int convert_dconst(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return __convert_fconst(J_DOUBLE,
				code[offset] - OPC_DCONST_0,
				cu->expr_stack);
}

int convert_bipush(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return __convert_const(J_INT, (char)code[offset + 1],
			       cu->expr_stack);
}

int convert_sipush(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return __convert_const(J_INT,
			       (short)be16_to_cpu(*(u2 *) & code[offset + 1]),
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

int convert_ldc(struct compilation_unit *cu, struct basic_block *bb,
		       unsigned long offset)
{
	struct classblock *cb = CLASS_CB(cu->method->class);
	unsigned char *code = cu->method->code;
	return __convert_ldc(&cb->constant_pool,
			     code[offset + 1], cu->expr_stack);
}

int convert_ldc_w(struct compilation_unit *cu, struct basic_block *bb,
			 unsigned long offset)
{
	struct classblock *cb = CLASS_CB(cu->method->class);
	unsigned char *code = cu->method->code;
	return __convert_ldc(&cb->constant_pool,
			     be16_to_cpu(*(u2 *) & code[offset + 1]),
			     cu->expr_stack);
}

int convert_ldc2_w(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	struct classblock *cb = CLASS_CB(cu->method->class);
	unsigned char *code = cu->method->code;
	return __convert_ldc(&cb->constant_pool,
			     be16_to_cpu(*(u2 *) & code[offset + 1]),
			     cu->expr_stack);
}

int convert_load(struct compilation_unit *cu,
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

int convert_iload(struct compilation_unit *cu, struct basic_block *bb,
			 unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_load(cu, bb, code[offset + 1], J_INT);
}

int convert_lload(struct compilation_unit *cu, struct basic_block *bb,
			 unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_load(cu, bb, code[offset + 1], J_LONG);
}

int convert_fload(struct compilation_unit *cu, struct basic_block *bb,
			 unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_load(cu, bb, code[offset + 1], J_FLOAT);
}

int convert_dload(struct compilation_unit *cu, struct basic_block *bb,
			 unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_load(cu, bb, code[offset + 1], J_DOUBLE);
}

int convert_aload(struct compilation_unit *cu, struct basic_block *bb,
			 unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_load(cu, bb, code[offset + 1], J_REFERENCE);
}

int convert_iload_n(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_load(cu, bb, code[offset] - OPC_ILOAD_0, J_INT);
}

int convert_lload_n(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_load(cu, bb, code[offset] - OPC_LLOAD_0, J_LONG);
}

int convert_fload_n(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_load(cu, bb, code[offset] - OPC_FLOAD_0, J_FLOAT);
}

int convert_dload_n(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_load(cu, bb, code[offset] - OPC_DLOAD_0, J_DOUBLE);
}

int convert_aload_n(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_load(cu, bb, code[offset] - OPC_ALOAD_0,
			    J_REFERENCE);
}

int convert_array_load(struct compilation_unit *cu,
			      struct basic_block *bb, enum jvm_type type)
{
	struct expression *index, *arrayref;
	struct expression *src_expr, *dest_expr;
	struct statement *store_stmt, *arraycheck, *nullcheck;

	index = stack_pop(cu->expr_stack);
	arrayref = stack_pop(cu->expr_stack);

	store_stmt = alloc_statement(STMT_STORE);
	if (!store_stmt)
		goto failed;

	src_expr = array_deref_expr(type, arrayref, index);
	dest_expr = temporary_expr(type, alloc_temporary());
	
	store_stmt->store_src = &src_expr->node;
	store_stmt->store_dest = &dest_expr->node;

	expr_get(dest_expr);
	stack_push(cu->expr_stack, dest_expr);

	arraycheck = alloc_statement(STMT_ARRAY_CHECK);
	if (!arraycheck)
		goto failed_arraycheck;

	expr_get(src_expr);
	arraycheck->expression = &src_expr->node;

	nullcheck = alloc_statement(STMT_NULL_CHECK);
	if (!nullcheck)
		goto failed_nullcheck;

	expr_get(arrayref);
	nullcheck->expression = &arrayref->node;

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

int convert_iaload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_INT);
}

int convert_laload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_LONG);
}

int convert_faload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_FLOAT);
}

int convert_daload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_DOUBLE);
}

int convert_aaload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_REFERENCE);
}

int convert_baload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_INT);
}

int convert_caload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_CHAR);
}

int convert_saload(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	return convert_array_load(cu, bb, J_SHORT);
}

int convert_store(struct compilation_unit *cu,
			 struct basic_block *bb,
			 enum jvm_type type, unsigned long index)
{
	struct expression *src_expr, *dest_expr;
	struct statement *stmt = alloc_statement(STMT_STORE);
	if (!stmt)
		goto failed;

	dest_expr = local_expr(type, index);
	src_expr = stack_pop(cu->expr_stack);

	stmt->store_dest = &dest_expr->node;
	stmt->store_src = &src_expr->node;
	bb_insert_stmt(bb, stmt);
	return 0;
      failed:
	free_statement(stmt);
	return -ENOMEM;
}

int convert_istore(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_store(cu, bb, J_INT, code[offset + 1]);
}

int convert_lstore(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_store(cu, bb, J_LONG, code[offset + 1]);
}

int convert_fstore(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_store(cu, bb, J_FLOAT, code[offset + 1]);
}

int convert_dstore(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_store(cu, bb, J_DOUBLE, code[offset + 1]);
}

int convert_astore(struct compilation_unit *cu, struct basic_block *bb,
			  unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_store(cu, bb, J_REFERENCE, code[offset + 1]);
}

int convert_istore_n(struct compilation_unit *cu, struct basic_block *bb,
			    unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_store(cu, bb, J_INT, code[offset] - OPC_ISTORE_0);
}

int convert_lstore_n(struct compilation_unit *cu, struct basic_block *bb,
			    unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_store(cu, bb, J_LONG, code[offset] - OPC_LSTORE_0);
}

int convert_fstore_n(struct compilation_unit *cu, struct basic_block *bb,
			    unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_store(cu, bb, J_FLOAT, code[offset] - OPC_FSTORE_0);
}

int convert_dstore_n(struct compilation_unit *cu, struct basic_block *bb,
			    unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_store(cu, bb, J_DOUBLE, code[offset] - OPC_DSTORE_0);
}

int convert_astore_n(struct compilation_unit *cu, struct basic_block *bb,
			    unsigned long offset)
{
	unsigned char *code = cu->method->code;
	return convert_store(cu, bb, J_REFERENCE,
			     code[offset] - OPC_ASTORE_0);
}

int convert_array_store(struct compilation_unit *cu,
			       struct basic_block *bb, enum jvm_type type)
{
	struct expression *value, *index, *arrayref;
	struct statement *store_stmt, *arraycheck, *nullcheck;
	struct expression *src_expr, *dest_expr;

	value = stack_pop(cu->expr_stack);
	index = stack_pop(cu->expr_stack);
	arrayref = stack_pop(cu->expr_stack);

	store_stmt = alloc_statement(STMT_STORE);
	if (!store_stmt)
		goto failed;

	dest_expr = array_deref_expr(type, arrayref, index);
	src_expr = value;

	store_stmt->store_dest = &dest_expr->node;
	store_stmt->store_src = &src_expr->node;

	arraycheck = alloc_statement(STMT_ARRAY_CHECK);
	if (!arraycheck)
		goto failed_arraycheck;

	expr_get(dest_expr);
	arraycheck->expression = &dest_expr->node;

	nullcheck = alloc_statement(STMT_NULL_CHECK);
	if (!nullcheck)
		goto failed_nullcheck;

	expr_get(arrayref);
	nullcheck->expression = &arrayref->node;

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

int convert_iastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_INT);
}

int convert_lastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_LONG);
}

int convert_fastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_FLOAT);
}

int convert_dastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_DOUBLE);
}

int convert_aastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_REFERENCE);
}

int convert_bastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_INT);
}

int convert_castore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_CHAR);
}

int convert_sastore(struct compilation_unit *cu, struct basic_block *bb,
			   unsigned long offset)
{
	return convert_array_store(cu, bb, J_SHORT);
}
