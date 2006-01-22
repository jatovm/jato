/*
 * Copyright (C) 2005  Pekka Enberg
 */

#include <statement.h>
#include <byteorder.h>
#include <stack.h>

#include <libharness.h>
#include <stdlib.h>

static void assert_value_expr(enum jvm_type expected_jvm_type,
			      long long expected_value,
			      struct expression *expression)
{
	assert_int_equals(EXPR_VALUE, expression->type);
	assert_int_equals(expected_jvm_type, expression->jvm_type);
	assert_int_equals(expected_value, expression->value);
}

static void assert_fvalue_expr(enum jvm_type expected_jvm_type,
			       double expected_value,
			       struct expression *expression)
{
	assert_int_equals(EXPR_FVALUE, expression->type);
	assert_int_equals(expected_jvm_type, expression->jvm_type);
	assert_float_equals(expected_value, expression->fvalue, 0.01f);
}

static void assert_temporary_expr(unsigned long expected,
				  struct expression *expression)
{
	assert_int_equals(EXPR_TEMPORARY, expression->type);
	assert_int_equals(expected, expression->temporary);
}

static void assert_array_deref_expr(struct expression *expected_arrayref,
				    struct expression *expected_index,
				    struct expression *expression)
{
	assert_int_equals(EXPR_ARRAY_DEREF, expression->type);
	assert_ptr_equals(expected_arrayref, expression->arrayref);
	assert_ptr_equals(expected_index, expression->array_index);
}

static void assert_binop_expr(enum jvm_type jvm_type,
			      enum operator operator,
			      struct expression *left,
			      struct expression *right,
			      struct expression *expression)
{
	assert_int_equals(EXPR_BINOP, expression->type);
	assert_int_equals(jvm_type, expression->jvm_type);
	assert_int_equals(operator, expression->operator);
	assert_ptr_equals(left, expression->left);
	assert_ptr_equals(right, expression->right);
}

static void __assert_const_expr_and_stack(struct classblock *cb,
					  enum statement_type
					  expected_stmt_type,
					  enum jvm_type
					  expected_jvm_type,
					  long long expected_value,
					  char *actual, size_t count)
{
	struct expression *expr;
	struct stack stack = STACK_INIT;

	convert_bytecode_to_stmts(cb, actual, count, &stack);

	expr = stack_pop(&stack);
	assert_value_expr(expected_jvm_type, expected_value, expr);
	assert_true(stack_is_empty(&stack));

	expr_put(expr);
}

static void assert_const_expr_and_stack(enum jvm_type expected_jvm_type,
					long long expected_value, char actual)
{
	unsigned char code[] = { actual };
	__assert_const_expr_and_stack(NULL, STMT_ASSIGN,
				      expected_jvm_type, expected_value,
				      code, sizeof(code));
}

static void assert_fconst_expr_and_stack(enum jvm_type expected_jvm_type,
					 double expected_value, char actual)
{
	struct expression *expr;
	struct stack stack = STACK_INIT;
	unsigned char code[] = { actual };

	convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);
	expr = stack_pop(&stack);
	assert_fvalue_expr(expected_jvm_type, expected_value, expr);
	assert_true(stack_is_empty(&stack));

	expr_put(expr);
}

void test_convert_nop(void)
{
	unsigned char code[] = { OPC_NOP };
	struct stack stack = STACK_INIT;
	struct statement *stmt =
	    convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);
	assert_int_equals(STMT_NOP, stmt->s_type);
	assert_true(stack_is_empty(&stack));
	free_stmt(stmt);
}

void test_convert_aconst_null(void)
{
	assert_const_expr_and_stack(J_REFERENCE, 0, OPC_ACONST_NULL);
}

void test_convert_iconst(void)
{
	assert_const_expr_and_stack(J_INT, -1, OPC_ICONST_M1);
	assert_const_expr_and_stack(J_INT, 0, OPC_ICONST_0);
	assert_const_expr_and_stack(J_INT, 1, OPC_ICONST_1);
	assert_const_expr_and_stack(J_INT, 2, OPC_ICONST_2);
	assert_const_expr_and_stack(J_INT, 3, OPC_ICONST_3);
	assert_const_expr_and_stack(J_INT, 4, OPC_ICONST_4);
	assert_const_expr_and_stack(J_INT, 5, OPC_ICONST_5);
}

void test_convert_lconst(void)
{
	assert_const_expr_and_stack(J_LONG, 0, OPC_LCONST_0);
	assert_const_expr_and_stack(J_LONG, 1, OPC_LCONST_1);
}

void test_convert_fconst(void)
{
	assert_fconst_expr_and_stack(J_FLOAT, 0, OPC_FCONST_0);
	assert_fconst_expr_and_stack(J_FLOAT, 1, OPC_FCONST_1);
	assert_fconst_expr_and_stack(J_FLOAT, 2, OPC_FCONST_2);
}

void test_convert_dconst(void)
{
	assert_fconst_expr_and_stack(J_DOUBLE, 0, OPC_DCONST_0);
	assert_fconst_expr_and_stack(J_DOUBLE, 1, OPC_DCONST_1);
}

static void assert_bipush_expr_and_stack(char expected_value, char actual)
{
	unsigned char code[] = { actual, expected_value };
	__assert_const_expr_and_stack(NULL, STMT_ASSIGN, J_INT,
				      expected_value, code, sizeof(code));
}

void test_convert_bipush(void)
{
	assert_bipush_expr_and_stack(0x00, OPC_BIPUSH);
	assert_bipush_expr_and_stack(0x01, OPC_BIPUSH);
	assert_bipush_expr_and_stack(0xFF, OPC_BIPUSH);
}

static void assert_sipush_expr_and_stack(long long expected_value,
					 char first, char second, char actual)
{
	unsigned char code[] = { actual, first, second };
	__assert_const_expr_and_stack(NULL, STMT_ASSIGN, J_INT,
				      expected_value, code, sizeof(code));
}

#define MIN_SHORT (-32768)
#define MAX_SHORT 32767

void test_convert_sipush(void)
{
	assert_sipush_expr_and_stack(0, 0x00, 0x00, OPC_SIPUSH);
	assert_sipush_expr_and_stack(1, 0x00, 0x01, OPC_SIPUSH);
	assert_sipush_expr_and_stack(MIN_SHORT, 0x80, 0x00, OPC_SIPUSH);
	assert_sipush_expr_and_stack(MAX_SHORT, 0x7F, 0xFF, OPC_SIPUSH);
}

static struct statement *convert_bytecode_with_cp(ConstantPoolEntry * cp_infos,
						  size_t nr_cp_infos,
						  u1 * cp_types,
						  unsigned char opcode,
						  unsigned char index1,
						  unsigned char index2,
						  struct stack *stack)
{
	struct classblock cb = {
		.constant_pool_count = sizeof(cp_infos),
		.constant_pool.info = cp_infos,
		.constant_pool.type = cp_types
	};
	unsigned char code[] = { opcode, index1, index2 };
	return convert_bytecode_to_stmts(&cb, code, sizeof(code), stack);
}

static void assert_ldc_expr_and_stack(enum jvm_type expected_jvm_type,
				      long expected_value, u1 cp_type)
{
	struct expression *expr;
	ConstantPoolEntry cp_infos[] = { cpu_to_be64(expected_value) };
	u1 cp_types[] = { cp_type };
	struct stack stack = STACK_INIT;

	convert_bytecode_with_cp(cp_infos, sizeof(cp_infos), cp_types,
				 OPC_LDC, 0x00, 0x00, &stack);
	expr = stack_pop(&stack);
	assert_value_expr(expected_jvm_type, expected_value, expr);
	assert_true(stack_is_empty(&stack));
	expr_put(expr);
}

static void assert_ldc_fexpr_and_stack(float expected_value)
{
	struct expression *expr;
	u4 value = *(u4 *) & expected_value;
	ConstantPoolEntry cp_infos[] = { cpu_to_be64(value) };
	u1 cp_types[] = { CONSTANT_Float };
	struct stack stack = STACK_INIT;

	convert_bytecode_with_cp(cp_infos, sizeof(cp_infos), cp_types,
				 OPC_LDC, 0x00, 0x00, &stack);
	expr = stack_pop(&stack);
	assert_fvalue_expr(J_FLOAT, expected_value, expr);
	assert_true(stack_is_empty(&stack));
	expr_put(expr);
}

#define INT_MAX 2147483647
#define INT_MIN (-INT_MAX - 1)

void test_convert_ldc(void)
{
	assert_ldc_expr_and_stack(J_INT, 0, CONSTANT_Integer);
	assert_ldc_expr_and_stack(J_INT, 1, CONSTANT_Integer);
	assert_ldc_expr_and_stack(J_INT, INT_MIN, CONSTANT_Integer);
	assert_ldc_expr_and_stack(J_INT, INT_MAX, CONSTANT_Integer);
	assert_ldc_fexpr_and_stack(0.01f);
	assert_ldc_fexpr_and_stack(1.0f);
	assert_ldc_fexpr_and_stack(-1.0f);
	assert_ldc_expr_and_stack(J_REFERENCE, 0xDEADBEEF, CONSTANT_String);
}

static void assert_ldcw_expr_and_stack(enum jvm_type expected_jvm_type,
				       long long expected_value, u1 cp_type,
				       unsigned char opcode)
{
	struct expression *expr;
	ConstantPoolEntry cp_infos[257];
	cp_infos[256] = cpu_to_be64(expected_value);
	u1 cp_types[257];
	cp_types[256] = cp_type;
	struct stack stack = STACK_INIT;

	convert_bytecode_with_cp(cp_infos, sizeof(cp_infos), cp_types, opcode,
				 0x01, 0x00, &stack);
	expr = stack_pop(&stack);
	assert_value_expr(expected_jvm_type, expected_value, expr);
	assert_true(stack_is_empty(&stack));
	expr_put(expr);
}

static void assert_ldcw_fexpr_and_stack(enum jvm_type expected_jvm_type,
					double expected_value,
					u1 cp_type, u8 value,
					unsigned long opcode)
{
	ConstantPoolEntry cp_infos[257];
	cp_infos[256] = cpu_to_be64(value);
	u1 cp_types[257];
	cp_types[256] = cp_type;
	struct stack stack = STACK_INIT;
	struct expression *expr;

	convert_bytecode_with_cp(cp_infos, sizeof(cp_infos), cp_types, opcode,
				 0x01, 0x00, &stack);
	expr = stack_pop(&stack);
	assert_fvalue_expr(expected_jvm_type, expected_value, expr);
	assert_true(stack_is_empty(&stack));
	expr_put(expr);
}

static void assert_ldcw_float_expr_and_stack(enum jvm_type expected_jvm_type,
					     float expected_value, u1 cp_type,
					     unsigned long opcode)
{
	u4 value = *(u4 *) & expected_value;
	assert_ldcw_fexpr_and_stack(expected_jvm_type,
				    expected_value, cp_type, value, opcode);
}

static void assert_ldcw_double_expr_and_stack(enum jvm_type
					      expected_jvm_type,
					      double expected_value,
					      u1 cp_type, unsigned long opcode)
{
	u8 value = *(u8 *) & expected_value;
	assert_ldcw_fexpr_and_stack(expected_jvm_type,
				    expected_value, cp_type, value, opcode);
}

void test_convert_ldc_w(void)
{
	assert_ldcw_expr_and_stack(J_INT, 0, CONSTANT_Integer, OPC_LDC_W);
	assert_ldcw_expr_and_stack(J_INT, 1, CONSTANT_Integer, OPC_LDC_W);
	assert_ldcw_expr_and_stack(J_INT, INT_MIN, CONSTANT_Integer, OPC_LDC_W);
	assert_ldcw_expr_and_stack(J_INT, INT_MAX, CONSTANT_Integer, OPC_LDC_W);
	assert_ldcw_float_expr_and_stack(J_FLOAT, 0.01f, CONSTANT_Float,
					 OPC_LDC_W);
	assert_ldcw_float_expr_and_stack(J_FLOAT, 1.0f, CONSTANT_Float,
					 OPC_LDC_W);
	assert_ldcw_float_expr_and_stack(J_FLOAT, -1.0f, CONSTANT_Float,
					 OPC_LDC_W);
	assert_ldcw_expr_and_stack(J_REFERENCE, 0xDEADBEEF, CONSTANT_String,
				   OPC_LDC_W);
}

#define LONG_MAX ((long long) 2<<63)
#define LONG_MIN (-LONG_MAX - 1)

void test_convert_ldc2_w(void)
{
	assert_ldcw_expr_and_stack(J_LONG, 0, CONSTANT_Long, OPC_LDC2_W);
	assert_ldcw_expr_and_stack(J_LONG, 1, CONSTANT_Long, OPC_LDC2_W);
	assert_ldcw_expr_and_stack(J_LONG, LONG_MIN, CONSTANT_Long, OPC_LDC2_W);
	assert_ldcw_expr_and_stack(J_LONG, LONG_MAX, CONSTANT_Long, OPC_LDC2_W);
	assert_ldcw_double_expr_and_stack(J_DOUBLE, 0.01f, CONSTANT_Double,
					  OPC_LDC2_W);
	assert_ldcw_double_expr_and_stack(J_DOUBLE, 1.0f, CONSTANT_Double,
					  OPC_LDC2_W);
	assert_ldcw_double_expr_and_stack(J_DOUBLE, -1.0f, CONSTANT_Double,
					  OPC_LDC2_W);
}

static void assert_load_stmt(unsigned char opc,
			     enum jvm_type expected_jvm_type,
			     unsigned char expected_index)
{
	unsigned char code[] = { opc, expected_index };
	struct stack stack = STACK_INIT;
	struct statement *stmt =
	    convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);
	assert_int_equals(STMT_ASSIGN, stmt->s_type);
	assert_int_equals(EXPR_LOCAL, stmt->s_left->type);
	assert_int_equals(expected_index, stmt->s_left->local_index);
	assert_int_equals(expected_jvm_type, stmt->s_left->jvm_type);
	assert_temporary_expr(stmt->s_target->temporary, stack_pop(&stack));
	assert_true(stack_is_empty(&stack));
	free_stmt(stmt);
}

void test_convert_iload(void)
{
	assert_load_stmt(OPC_ILOAD, J_INT, 0x00);
	assert_load_stmt(OPC_ILOAD, J_INT, 0x01);
	assert_load_stmt(OPC_ILOAD, J_INT, 0xFF);
}

void test_convert_lload(void)
{
	assert_load_stmt(OPC_LLOAD, J_LONG, 0x00);
	assert_load_stmt(OPC_LLOAD, J_LONG, 0x01);
	assert_load_stmt(OPC_LLOAD, J_LONG, 0xFF);
}

void test_convert_fload(void)
{
	assert_load_stmt(OPC_FLOAD, J_FLOAT, 0x00);
	assert_load_stmt(OPC_FLOAD, J_FLOAT, 0x01);
	assert_load_stmt(OPC_FLOAD, J_FLOAT, 0xFF);
}

void test_convert_dload(void)
{
	assert_load_stmt(OPC_DLOAD, J_DOUBLE, 0x00);
	assert_load_stmt(OPC_DLOAD, J_DOUBLE, 0x01);
	assert_load_stmt(OPC_DLOAD, J_DOUBLE, 0xFF);
}

void test_convert_aload(void)
{
	assert_load_stmt(OPC_ALOAD, J_REFERENCE, 0x00);
	assert_load_stmt(OPC_ALOAD, J_REFERENCE, 0x01);
	assert_load_stmt(OPC_ALOAD, J_REFERENCE, 0xFF);
}

void test_convert_iload_n(void)
{
	assert_load_stmt(OPC_ILOAD_0, J_INT, 0x00);
	assert_load_stmt(OPC_ILOAD_1, J_INT, 0x01);
	assert_load_stmt(OPC_ILOAD_2, J_INT, 0x02);
	assert_load_stmt(OPC_ILOAD_3, J_INT, 0x03);
}

void test_convert_lload_n(void)
{
	assert_load_stmt(OPC_LLOAD_0, J_LONG, 0x00);
	assert_load_stmt(OPC_LLOAD_1, J_LONG, 0x01);
	assert_load_stmt(OPC_LLOAD_2, J_LONG, 0x02);
	assert_load_stmt(OPC_LLOAD_3, J_LONG, 0x03);
}

void test_convert_fload_n(void)
{
	assert_load_stmt(OPC_FLOAD_0, J_FLOAT, 0x00);
	assert_load_stmt(OPC_FLOAD_1, J_FLOAT, 0x01);
	assert_load_stmt(OPC_FLOAD_2, J_FLOAT, 0x02);
	assert_load_stmt(OPC_FLOAD_3, J_FLOAT, 0x03);
}

void test_convert_dload_n(void)
{
	assert_load_stmt(OPC_DLOAD_0, J_DOUBLE, 0x00);
	assert_load_stmt(OPC_DLOAD_1, J_DOUBLE, 0x01);
	assert_load_stmt(OPC_DLOAD_2, J_DOUBLE, 0x02);
	assert_load_stmt(OPC_DLOAD_3, J_DOUBLE, 0x03);
}

void test_convert_aload_n(void)
{
	assert_load_stmt(OPC_ALOAD_0, J_REFERENCE, 0x00);
	assert_load_stmt(OPC_ALOAD_1, J_REFERENCE, 0x01);
	assert_load_stmt(OPC_ALOAD_2, J_REFERENCE, 0x02);
	assert_load_stmt(OPC_ALOAD_3, J_REFERENCE, 0x03);
}

static void assert_null_check_stmt(struct expression *expected,
				   struct statement *actual)
{
	assert_int_equals(STMT_NULL_CHECK, actual->s_type);
	assert_value_expr(J_REFERENCE, expected->value, actual->s_left);
}

static void assert_arraycheck_stmt(struct expression *expected_arrayref,
				   struct expression *expected_index,
				   struct statement *actual)
{
	assert_int_equals(STMT_ARRAY_CHECK, actual->s_type);
	assert_array_deref_expr(expected_arrayref, expected_index,
				actual->s_left);
}

static void assert_array_load_stmts(enum jvm_type expected_type,
				    unsigned char opc,
				    unsigned long arrayref, unsigned long index)
{
	unsigned char code[] = { opc };
	struct stack stack = STACK_INIT;
	struct expression *arrayref_expr, *index_expr, *temporary_expr;

	arrayref_expr = value_expr(J_REFERENCE, arrayref);
	index_expr = value_expr(J_INT, index);

	stack_push(&stack, arrayref_expr);
	stack_push(&stack, index_expr);

	struct statement *stmt =
	    convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);

	struct statement *nullcheck = stmt;
	struct statement *arraycheck = stmt->s_next;
	struct statement *assign = arraycheck->s_next;

	assert_null_check_stmt(arrayref_expr, nullcheck);
	assert_arraycheck_stmt(arrayref_expr, index_expr, arraycheck);

	assert_int_equals(STMT_ASSIGN, assign->s_type);
	assert_int_equals(expected_type, assign->s_left->jvm_type);
	assert_array_deref_expr(arrayref_expr, index_expr, assign->s_left);

	temporary_expr = stack_pop(&stack);
	assert_temporary_expr(assign->s_target->temporary, temporary_expr);
	expr_put(temporary_expr);
	assert_true(stack_is_empty(&stack));

	free_stmt(stmt);
}

void test_convert_iaload(void)
{
	assert_array_load_stmts(J_INT, OPC_IALOAD, 0, 1);
	assert_array_load_stmts(J_INT, OPC_IALOAD, 1, 2);
}

void test_convert_laload(void)
{
	assert_array_load_stmts(J_LONG, OPC_LALOAD, 0, 1);
	assert_array_load_stmts(J_LONG, OPC_LALOAD, 1, 2);
}

void test_convert_faload(void)
{
	assert_array_load_stmts(J_FLOAT, OPC_FALOAD, 0, 1);
	assert_array_load_stmts(J_FLOAT, OPC_FALOAD, 1, 2);
}

void test_convert_daload(void)
{
	assert_array_load_stmts(J_DOUBLE, OPC_DALOAD, 0, 1);
	assert_array_load_stmts(J_DOUBLE, OPC_DALOAD, 1, 2);
}

void test_convert_aaload(void)
{
	assert_array_load_stmts(J_REFERENCE, OPC_AALOAD, 0, 1);
	assert_array_load_stmts(J_REFERENCE, OPC_AALOAD, 1, 2);
}

void test_convert_baload(void)
{
	assert_array_load_stmts(J_INT, OPC_BALOAD, 0, 1);
	assert_array_load_stmts(J_INT, OPC_BALOAD, 1, 2);
}

void test_convert_caload(void)
{
	assert_array_load_stmts(J_CHAR, OPC_CALOAD, 0, 1);
	assert_array_load_stmts(J_CHAR, OPC_CALOAD, 1, 2);
}

void test_convert_saload(void)
{
	assert_array_load_stmts(J_SHORT, OPC_SALOAD, 0, 1);
	assert_array_load_stmts(J_SHORT, OPC_SALOAD, 1, 2);
}

static void assert_store_stmt(unsigned char opc,
			      enum jvm_type expected_jvm_type,
			      unsigned char expected_index,
			      unsigned long expected_temporary)
{
	unsigned char code[] = { opc, expected_index };
	struct stack stack = STACK_INIT;

	stack_push(&stack, temporary_expr(J_INT, expected_temporary));

	struct statement *stmt =
	    convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);

	assert_int_equals(STMT_ASSIGN, stmt->s_type);

	assert_int_equals(EXPR_TEMPORARY, stmt->s_left->type);
	assert_int_equals(expected_temporary, stmt->s_left->temporary);

	assert_int_equals(EXPR_LOCAL, stmt->s_target->type);
	assert_int_equals(expected_index, stmt->s_target->local_index);
	assert_int_equals(expected_jvm_type, stmt->s_target->jvm_type);

	assert_true(stack_is_empty(&stack));

	free_stmt(stmt);
}

void test_convert_istore(void)
{
	assert_store_stmt(OPC_ISTORE, J_INT, 0x00, 0x01);
	assert_store_stmt(OPC_ISTORE, J_INT, 0x01, 0x02);
}

void test_convert_lstore(void)
{
	assert_store_stmt(OPC_LSTORE, J_LONG, 0x00, 0x01);
	assert_store_stmt(OPC_LSTORE, J_LONG, 0x01, 0x02);
}

void test_convert_fstore(void)
{
	assert_store_stmt(OPC_FSTORE, J_FLOAT, 0x00, 0x01);
	assert_store_stmt(OPC_FSTORE, J_FLOAT, 0x01, 0x02);
}

void test_convert_dstore(void)
{
	assert_store_stmt(OPC_DSTORE, J_DOUBLE, 0x00, 0x01);
	assert_store_stmt(OPC_DSTORE, J_DOUBLE, 0x01, 0x02);
}

void test_convert_astore(void)
{
	assert_store_stmt(OPC_ASTORE, J_REFERENCE, 0x00, 0x01);
	assert_store_stmt(OPC_ASTORE, J_REFERENCE, 0x01, 0x02);
}

void test_convert_istore_n(void)
{
	assert_store_stmt(OPC_ISTORE_0, J_INT, 0x00, 0xFF);
	assert_store_stmt(OPC_ISTORE_1, J_INT, 0x01, 0xFF);
	assert_store_stmt(OPC_ISTORE_2, J_INT, 0x02, 0xFF);
	assert_store_stmt(OPC_ISTORE_3, J_INT, 0x03, 0xFF);
}

void test_convert_lstore_n(void)
{
	assert_store_stmt(OPC_LSTORE_0, J_LONG, 0x00, 0xFF);
	assert_store_stmt(OPC_LSTORE_1, J_LONG, 0x01, 0xFF);
	assert_store_stmt(OPC_LSTORE_2, J_LONG, 0x02, 0xFF);
	assert_store_stmt(OPC_LSTORE_3, J_LONG, 0x03, 0xFF);
}

void test_convert_fstore_n(void)
{
	assert_store_stmt(OPC_FSTORE_0, J_FLOAT, 0x00, 0xFF);
	assert_store_stmt(OPC_FSTORE_1, J_FLOAT, 0x01, 0xFF);
	assert_store_stmt(OPC_FSTORE_2, J_FLOAT, 0x02, 0xFF);
	assert_store_stmt(OPC_FSTORE_3, J_FLOAT, 0x03, 0xFF);
}

void test_convert_dstore_n(void)
{
	assert_store_stmt(OPC_DSTORE_0, J_DOUBLE, 0x00, 0xFF);
	assert_store_stmt(OPC_DSTORE_1, J_DOUBLE, 0x01, 0xFF);
	assert_store_stmt(OPC_DSTORE_2, J_DOUBLE, 0x02, 0xFF);
	assert_store_stmt(OPC_DSTORE_3, J_DOUBLE, 0x03, 0xFF);
}

void test_convert_astore_n(void)
{
	assert_store_stmt(OPC_ASTORE_0, J_REFERENCE, 0x00, 0xFF);
	assert_store_stmt(OPC_ASTORE_1, J_REFERENCE, 0x01, 0xFF);
	assert_store_stmt(OPC_ASTORE_2, J_REFERENCE, 0x02, 0xFF);
	assert_store_stmt(OPC_ASTORE_3, J_REFERENCE, 0x03, 0xFF);
}

static void assert_array_store_stmts(enum jvm_type expected_type,
				     unsigned char opc,
				     unsigned long arrayref,
				     unsigned long index, unsigned long value)
{
	unsigned char code[] = { opc };
	struct stack stack = STACK_INIT;
	struct expression *arrayref_expr, *index_expr, *expr;

	arrayref_expr = value_expr(J_REFERENCE, arrayref);
	index_expr = value_expr(J_INT, index);
	expr = temporary_expr(expected_type, value);

	stack_push(&stack, arrayref_expr);
	stack_push(&stack, index_expr);
	stack_push(&stack, expr);

	struct statement *stmt =
	    convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);

	struct statement *nullcheck = stmt;
	struct statement *arraycheck = nullcheck->s_next;
	struct statement *assign = arraycheck->s_next;

	assert_null_check_stmt(arrayref_expr, nullcheck);
	assert_arraycheck_stmt(arrayref_expr, index_expr, arraycheck);

	assert_int_equals(STMT_ASSIGN, assign->s_type);
	assert_int_equals(expected_type, assign->s_target->jvm_type);
	assert_array_deref_expr(arrayref_expr, index_expr, assign->s_target);
	assert_temporary_expr(value, assign->s_left);

	assert_true(stack_is_empty(&stack));
	free_stmt(stmt);
}

void test_convert_iastore(void)
{
	assert_array_store_stmts(J_INT, OPC_IASTORE, 0, 1, 2);
	assert_array_store_stmts(J_INT, OPC_IASTORE, 2, 3, 4);
}

void test_convert_lastore(void)
{
	assert_array_store_stmts(J_LONG, OPC_LASTORE, 0, 1, 2);
	assert_array_store_stmts(J_LONG, OPC_LASTORE, 2, 3, 4);
}

void test_convert_fastore(void)
{
	assert_array_store_stmts(J_FLOAT, OPC_FASTORE, 0, 1, 2);
	assert_array_store_stmts(J_FLOAT, OPC_FASTORE, 2, 3, 4);
}

void test_convert_dastore(void)
{
	assert_array_store_stmts(J_DOUBLE, OPC_DASTORE, 0, 1, 2);
	assert_array_store_stmts(J_DOUBLE, OPC_DASTORE, 2, 3, 4);
}

void test_convert_aastore(void)
{
	assert_array_store_stmts(J_REFERENCE, OPC_AASTORE, 0, 1, 2);
	assert_array_store_stmts(J_REFERENCE, OPC_AASTORE, 2, 3, 4);
}

void test_convert_bastore(void)
{
	assert_array_store_stmts(J_INT, OPC_BASTORE, 0, 1, 2);
	assert_array_store_stmts(J_INT, OPC_BASTORE, 2, 3, 4);
}

void test_convert_castore(void)
{
	assert_array_store_stmts(J_CHAR, OPC_CASTORE, 0, 1, 2);
	assert_array_store_stmts(J_CHAR, OPC_CASTORE, 2, 3, 4);
}

void test_convert_sastore(void)
{
	assert_array_store_stmts(J_SHORT, OPC_SASTORE, 0, 1, 2);
	assert_array_store_stmts(J_SHORT, OPC_SASTORE, 2, 3, 4);
}

static void assert_pop_stack(unsigned char opc)
{
	unsigned char code[] = { opc };
	struct stack stack = STACK_INIT;
	stack_push(&stack, (void *)1);
	convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);
	assert_true(stack_is_empty(&stack));
}

void test_convert_pop(void)
{
	assert_pop_stack(OPC_POP);
	assert_pop_stack(OPC_POP2);
}

static void assert_dup_stack(unsigned char opc, void *expected)
{
	unsigned char code[] = { opc };
	struct stack stack = STACK_INIT;
	stack_push(&stack, expected);
	convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);
	assert_ptr_equals(stack_pop(&stack), expected);
	assert_ptr_equals(stack_pop(&stack), expected);
	assert_true(stack_is_empty(&stack));
}

void test_convert_dup(void)
{
	assert_dup_stack(OPC_DUP, (void *)1);
	assert_dup_stack(OPC_DUP, (void *)2);
	assert_dup_stack(OPC_DUP2, (void *)1);
	assert_dup_stack(OPC_DUP2, (void *)2);
}

static void assert_dup_x1_stack(unsigned char opc,
				void *expected1, void *expected2)
{
	unsigned char code[] = { opc };
	struct stack stack = STACK_INIT;
	stack_push(&stack, expected2);
	stack_push(&stack, expected1);
	convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);
	assert_ptr_equals(stack_pop(&stack), expected1);
	assert_ptr_equals(stack_pop(&stack), expected2);
	assert_ptr_equals(stack_pop(&stack), expected1);
	assert_true(stack_is_empty(&stack));
}

void test_convert_dup_x1(void)
{
	assert_dup_x1_stack(OPC_DUP_X1, (void *)1, (void *)2);
	assert_dup_x1_stack(OPC_DUP_X1, (void *)2, (void *)3);
	assert_dup_x1_stack(OPC_DUP2_X1, (void *)1, (void *)2);
	assert_dup_x1_stack(OPC_DUP2_X1, (void *)2, (void *)3);
}

static void assert_dup_x2_stack(unsigned char opc,
				void *expected1, void *expected2,
				void *expected3)
{
	unsigned char code[] = { opc };
	struct stack stack = STACK_INIT;
	stack_push(&stack, expected3);
	stack_push(&stack, expected2);
	stack_push(&stack, expected1);
	convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);
	assert_ptr_equals(stack_pop(&stack), expected1);
	assert_ptr_equals(stack_pop(&stack), expected2);
	assert_ptr_equals(stack_pop(&stack), expected3);
	assert_ptr_equals(stack_pop(&stack), expected1);
	assert_true(stack_is_empty(&stack));
}

void test_convert_dup_x2(void)
{
	assert_dup_x2_stack(OPC_DUP_X2, (void *)1, (void *)2, (void *)3);
	assert_dup_x2_stack(OPC_DUP_X2, (void *)2, (void *)3, (void *)4);
	assert_dup_x2_stack(OPC_DUP2_X2, (void *)1, (void *)2, (void *)3);
	assert_dup_x2_stack(OPC_DUP2_X2, (void *)2, (void *)3, (void *)4);
}

static void assert_swap_stack(unsigned char opc,
			      void *expected1, void *expected2)
{
	unsigned char code[] = { opc };
	struct stack stack = STACK_INIT;
	stack_push(&stack, expected1);
	stack_push(&stack, expected2);

	convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);
	assert_ptr_equals(stack_pop(&stack), expected1);
	assert_ptr_equals(stack_pop(&stack), expected2);
	assert_true(stack_is_empty(&stack));
}

void test_convert_swap(void)
{
	assert_swap_stack(OPC_SWAP, (void *)1, (void *)2);
	assert_swap_stack(OPC_SWAP, (void *)2, (void *)3);
}

static void assert_binop_expr_and_stack(enum jvm_type jvm_type,
					enum operator operator,
					unsigned char opc)
{
	unsigned char code[] = { opc };
	struct stack stack = STACK_INIT;
	struct expression *left, *right, *expr;
	struct statement *stmt;

	left = temporary_expr(jvm_type, 1);
	right = temporary_expr(jvm_type, 2);

	stack_push(&stack, left);
	stack_push(&stack, right);

	stmt = convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);
	expr = stack_pop(&stack);

	assert_binop_expr(jvm_type, operator, left, right, expr);
	assert_true(stack_is_empty(&stack));

	expr_put(expr);
}

void test_convert_add(void)
{
	assert_binop_expr_and_stack(J_INT, OP_ADD, OPC_IADD);
	assert_binop_expr_and_stack(J_LONG, OP_ADD, OPC_LADD);
	assert_binop_expr_and_stack(J_FLOAT, OP_ADD, OPC_FADD);
	assert_binop_expr_and_stack(J_DOUBLE, OP_ADD, OPC_DADD);
}

void test_convert_sub(void)
{
	assert_binop_expr_and_stack(J_INT, OP_SUB, OPC_ISUB);
	assert_binop_expr_and_stack(J_LONG, OP_SUB, OPC_LSUB);
	assert_binop_expr_and_stack(J_FLOAT, OP_SUB, OPC_FSUB);
	assert_binop_expr_and_stack(J_DOUBLE, OP_SUB, OPC_DSUB);
}

void test_convert_mul(void)
{
	assert_binop_expr_and_stack(J_INT, OP_MUL, OPC_IMUL);
	assert_binop_expr_and_stack(J_LONG, OP_MUL, OPC_LMUL);
	assert_binop_expr_and_stack(J_FLOAT, OP_MUL, OPC_FMUL);
	assert_binop_expr_and_stack(J_DOUBLE, OP_MUL, OPC_DMUL);
}

void test_convert_div(void)
{
	assert_binop_expr_and_stack(J_INT, OP_DIV, OPC_IDIV);
	assert_binop_expr_and_stack(J_LONG, OP_DIV, OPC_LDIV);
	assert_binop_expr_and_stack(J_FLOAT, OP_DIV, OPC_FDIV);
	assert_binop_expr_and_stack(J_DOUBLE, OP_DIV, OPC_DDIV);
}

void test_convert_rem(void)
{
	assert_binop_expr_and_stack(J_INT, OP_REM, OPC_IREM);
	assert_binop_expr_and_stack(J_LONG, OP_REM, OPC_LREM);
	assert_binop_expr_and_stack(J_FLOAT, OP_REM, OPC_FREM);
	assert_binop_expr_and_stack(J_DOUBLE, OP_REM, OPC_DREM);
}
