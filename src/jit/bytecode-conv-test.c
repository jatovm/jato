/*
 * Copyright (C) 2005  Pekka Enberg
 */

#include <statement.h>
#include <byteorder.h>
#include <stack.h>

#include <CuTest.h>
#include <stdlib.h>

static void assert_const_expression(CuTest * ct,
				 enum jvm_type expected_const_type,
				 long long expected_value,
				 struct expression *expression)
{
	CuAssertIntEquals(ct, EXPR_VALUE, expression->type);
	CuAssertIntEquals(ct, expected_const_type, expression->jvm_type);
	CuAssertIntEquals(ct, expected_value, expression->value);
}

static void assert_fconst_expression(CuTest * ct,
				  enum jvm_type expected_const_type,
				  double expected_value,
				  struct expression *expression)
{
	CuAssertIntEquals(ct, EXPR_FVALUE, expression->type);
	CuAssertIntEquals(ct, expected_const_type, expression->jvm_type);
	CuAssertDblEquals(ct, expected_value, expression->fvalue, 0.01f);
}

static void assert_temporary_expression(CuTest * ct, unsigned long expected,
				     struct expression *expression)
{
	CuAssertIntEquals(ct, TEMPORARY, expression->type);
	CuAssertIntEquals(ct, expected, expression->temporary);
}

static void assert_arrayref_expression(CuTest * ct,
				    unsigned long expected_arrayref,
				    unsigned long expected_index,
				    struct expression *expression)
{
	CuAssertIntEquals(ct, ARRAYREF, expression->type);
	CuAssertIntEquals(ct, expected_arrayref, expression->arrayref);
	CuAssertIntEquals(ct, expected_index, expression->array_index);
}

static void __assert_const_stmt(CuTest * ct, struct classblock *cb,
				enum statement_type
				expected_stmt_type,
				enum jvm_type
				expected_const_type,
				long long expected_value,
				char *actual, size_t count)
{
	struct stack stack = STACK_INIT;
	struct statement *stmt =
	    convert_bytecode_to_stmts(cb, actual, count, &stack);
	CuAssertIntEquals(ct, expected_stmt_type, stmt->s_type);
	assert_const_expression(ct, expected_const_type, expected_value,
			     stmt->s_left);

	CuAssertIntEquals(ct, stack_pop(&stack), stmt->s_target->temporary);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));

	free_stmt(stmt);
}

static void assert_const_stmt(CuTest * ct,
			      enum jvm_type expected_const_type,
			      long long expected_value, char actual)
{
	unsigned char code[] = { actual };
	__assert_const_stmt(ct, NULL, STMT_ASSIGN,
			    expected_const_type, expected_value,
			    code, sizeof(code));
}

static void assert_fconst_stmt(CuTest * ct,
			       enum jvm_type expected_const_type,
			       double expected_value, char actual)
{
	unsigned char code[] = { actual };
	struct stack stack = STACK_INIT;
	struct statement *stmt =
	    convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);
	CuAssertIntEquals(ct, STMT_ASSIGN, stmt->s_type);
	assert_fconst_expression(ct, expected_const_type, expected_value,
			      stmt->s_left);

	CuAssertIntEquals(ct, stack_pop(&stack), stmt->s_target->temporary);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));

	free_stmt(stmt);
}

void test_convert_nop(CuTest * ct)
{
	unsigned char code[] = { OPC_NOP };
	struct stack stack = STACK_INIT;
	struct statement *stmt =
	    convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);
	CuAssertIntEquals(ct, STMT_NOP, stmt->s_type);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free_stmt(stmt);
}

void test_convert_aconst_null(CuTest * ct)
{
	assert_const_stmt(ct, J_REFERENCE, 0, OPC_ACONST_NULL);
}

void test_convert_iconst(CuTest * ct)
{
	assert_const_stmt(ct, J_INT, -1, OPC_ICONST_M1);
	assert_const_stmt(ct, J_INT, 0, OPC_ICONST_0);
	assert_const_stmt(ct, J_INT, 1, OPC_ICONST_1);
	assert_const_stmt(ct, J_INT, 2, OPC_ICONST_2);
	assert_const_stmt(ct, J_INT, 3, OPC_ICONST_3);
	assert_const_stmt(ct, J_INT, 4, OPC_ICONST_4);
	assert_const_stmt(ct, J_INT, 5, OPC_ICONST_5);
}

void test_convert_lconst(CuTest * ct)
{
	assert_const_stmt(ct, J_LONG, 0, OPC_LCONST_0);
	assert_const_stmt(ct, J_LONG, 1, OPC_LCONST_1);
}

void test_convert_fconst(CuTest * ct)
{
	assert_fconst_stmt(ct, J_FLOAT, 0, OPC_FCONST_0);
	assert_fconst_stmt(ct, J_FLOAT, 1, OPC_FCONST_1);
	assert_fconst_stmt(ct, J_FLOAT, 2, OPC_FCONST_2);
}

void test_convert_dconst(CuTest * ct)
{
	assert_fconst_stmt(ct, J_DOUBLE, 0, OPC_DCONST_0);
	assert_fconst_stmt(ct, J_DOUBLE, 1, OPC_DCONST_1);
}

static void assert_bipush_stmt(CuTest * ct, char expected_value, char actual)
{
	unsigned char code[] = { actual, expected_value };
	__assert_const_stmt(ct, NULL, STMT_ASSIGN, J_INT,
			    expected_value, code, sizeof(code));
}

void test_convert_bipush(CuTest * ct)
{
	assert_bipush_stmt(ct, 0x00, OPC_BIPUSH);
	assert_bipush_stmt(ct, 0x01, OPC_BIPUSH);
	assert_bipush_stmt(ct, 0xFF, OPC_BIPUSH);
}

static void assert_sipush_stmt(CuTest * ct,
			       long long expected_value,
			       char first, char second, char actual)
{
	unsigned char code[] = { actual, first, second };
	__assert_const_stmt(ct, NULL, STMT_ASSIGN, J_INT,
			    expected_value, code, sizeof(code));
}

#define MIN_SHORT (-32768)
#define MAX_SHORT 32767

void test_convert_sipush(CuTest * ct)
{
	assert_sipush_stmt(ct, 0, 0x00, 0x00, OPC_SIPUSH);
	assert_sipush_stmt(ct, 1, 0x00, 0x01, OPC_SIPUSH);
	assert_sipush_stmt(ct, MIN_SHORT, 0x80, 0x00, OPC_SIPUSH);
	assert_sipush_stmt(ct, MAX_SHORT, 0x7F, 0xFF, OPC_SIPUSH);
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

static void assert_ldc_stmt(CuTest * ct,
			    enum jvm_type expected_const_type,
			    long expected_value, u1 cp_type)
{
	ConstantPoolEntry cp_infos[] = { cpu_to_be64(expected_value) };
	u1 cp_types[] = { cp_type };
	struct stack stack = STACK_INIT;

	struct statement *stmt =
	    convert_bytecode_with_cp(cp_infos, sizeof(cp_infos),
				     cp_types, OPC_LDC, 0x00,
				     0x00,
				     &stack);
	CuAssertIntEquals(ct, STMT_ASSIGN, stmt->s_type);
	assert_const_expression(ct, expected_const_type, expected_value,
			     stmt->s_left);
	CuAssertIntEquals(ct, stack_pop(&stack), stmt->s_target->temporary);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free_stmt(stmt);
}

static void assert_ldc_stmt_float(CuTest * ct, float expected_value)
{
	u4 value = *(u4 *) & expected_value;
	ConstantPoolEntry cp_infos[] = { cpu_to_be64(value) };
	u1 cp_types[] = { CONSTANT_Float };
	struct stack stack = STACK_INIT;

	struct statement *stmt =
	    convert_bytecode_with_cp(cp_infos, sizeof(cp_infos),
				     cp_types, OPC_LDC, 0x00,
				     0x00,
				     &stack);
	CuAssertIntEquals(ct, STMT_ASSIGN, stmt->s_type);
	assert_fconst_expression(ct, J_FLOAT, expected_value, stmt->s_left);
	CuAssertIntEquals(ct, stack_pop(&stack), stmt->s_target->temporary);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free_stmt(stmt);
}

#define INT_MAX 2147483647
#define INT_MIN (-INT_MAX - 1)

void test_convert_ldc(CuTest * ct)
{
	assert_ldc_stmt(ct, J_INT, 0, CONSTANT_Integer);
	assert_ldc_stmt(ct, J_INT, 1, CONSTANT_Integer);
	assert_ldc_stmt(ct, J_INT, INT_MIN, CONSTANT_Integer);
	assert_ldc_stmt(ct, J_INT, INT_MAX, CONSTANT_Integer);
	assert_ldc_stmt_float(ct, 0.01f);
	assert_ldc_stmt_float(ct, 1.0f);
	assert_ldc_stmt_float(ct, -1.0f);
	assert_ldc_stmt(ct, J_REFERENCE, 0xDEADBEEF, CONSTANT_String);
}

static void assert_ldcw_stmt(CuTest * ct,
			     enum jvm_type expected_const_type,
			     long long expected_value, u1 cp_type,
			     unsigned char opcode)
{
	ConstantPoolEntry cp_infos[257];
	cp_infos[256] = cpu_to_be64(expected_value);
	u1 cp_types[257];
	cp_types[256] = cp_type;
	struct stack stack = STACK_INIT;

	struct statement *stmt =
	    convert_bytecode_with_cp(cp_infos, sizeof(cp_infos),
				     cp_types, opcode, 0x01,
				     0x00,
				     &stack);
	CuAssertIntEquals(ct, STMT_ASSIGN, stmt->s_type);
	assert_const_expression(ct, expected_const_type, expected_value,
			     stmt->s_left);
	CuAssertIntEquals(ct, stack_pop(&stack), stmt->s_target->temporary);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free_stmt(stmt);
}

static void __assert_ldcw_stmt_double(CuTest * ct,
				      enum jvm_type
				      expected_jvm_type,
				      double expected_value,
				      u1 cp_type, u8 value,
				      unsigned long opcode)
{
	ConstantPoolEntry cp_infos[257];
	cp_infos[256] = cpu_to_be64(value);
	u1 cp_types[257];
	cp_types[256] = cp_type;
	struct stack stack = STACK_INIT;

	struct statement *stmt =
	    convert_bytecode_with_cp(cp_infos, sizeof(cp_infos),
				     cp_types, opcode, 0x01,
				     0x00,
				     &stack);
	CuAssertIntEquals(ct, STMT_ASSIGN, stmt->s_type);
	assert_fconst_expression(ct, expected_jvm_type, expected_value,
			      stmt->s_left);
	CuAssertIntEquals(ct, stack_pop(&stack), stmt->s_target->temporary);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free_stmt(stmt);
}

static void assert_ldcw_stmt_float(CuTest * ct,
				   enum jvm_type expected_jvm_type,
				   float expected_value, u1 cp_type,
				   unsigned long opcode)
{
	u4 value = *(u4 *) & expected_value;
	__assert_ldcw_stmt_double(ct, expected_jvm_type,
				  expected_value, cp_type, value, opcode);
}

static void assert_ldcw_stmt_double(CuTest * ct,
				    enum jvm_type
				    expected_jvm_type,
				    double expected_value,
				    u1 cp_type, unsigned long opcode)
{
	u8 value = *(u8 *) & expected_value;
	__assert_ldcw_stmt_double(ct, expected_jvm_type,
				  expected_value, cp_type, value, opcode);
}

void test_convert_ldc_w(CuTest * ct)
{
	assert_ldcw_stmt(ct, J_INT, 0, CONSTANT_Integer, OPC_LDC_W);
	assert_ldcw_stmt(ct, J_INT, 1, CONSTANT_Integer, OPC_LDC_W);
	assert_ldcw_stmt(ct, J_INT, INT_MIN, CONSTANT_Integer, OPC_LDC_W);
	assert_ldcw_stmt(ct, J_INT, INT_MAX, CONSTANT_Integer, OPC_LDC_W);
	assert_ldcw_stmt_float(ct, J_FLOAT, 0.01f, CONSTANT_Float,
			       OPC_LDC_W);
	assert_ldcw_stmt_float(ct, J_FLOAT, 1.0f, CONSTANT_Float,
			       OPC_LDC_W);
	assert_ldcw_stmt_float(ct, J_FLOAT, -1.0f, CONSTANT_Float,
			       OPC_LDC_W);
	assert_ldcw_stmt(ct, J_REFERENCE, 0xDEADBEEF, CONSTANT_String,
			 OPC_LDC_W);
}

#define LONG_MAX ((long long) 2<<63)
#define LONG_MIN (-LONG_MAX - 1)

void test_convert_ldc2_w(CuTest * ct)
{
	assert_ldcw_stmt(ct, J_LONG, 0, CONSTANT_Long, OPC_LDC2_W);
	assert_ldcw_stmt(ct, J_LONG, 1, CONSTANT_Long, OPC_LDC2_W);
	assert_ldcw_stmt(ct, J_LONG, LONG_MIN, CONSTANT_Long, OPC_LDC2_W);
	assert_ldcw_stmt(ct, J_LONG, LONG_MAX, CONSTANT_Long, OPC_LDC2_W);
	assert_ldcw_stmt_double(ct, J_DOUBLE, 0.01f, CONSTANT_Double,
				OPC_LDC2_W);
	assert_ldcw_stmt_double(ct, J_DOUBLE, 1.0f, CONSTANT_Double,
				OPC_LDC2_W);
	assert_ldcw_stmt_double(ct, J_DOUBLE, -1.0f, CONSTANT_Double,
				OPC_LDC2_W);
}

static void assert_load_stmt(CuTest * ct, unsigned char opc,
			      enum jvm_type expected_jvm_type,
			      unsigned char expected_index)
{
	unsigned char code[] = { opc, expected_index };
	struct stack stack = STACK_INIT;
	struct statement *stmt =
	    convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);
	CuAssertIntEquals(ct, STMT_ASSIGN, stmt->s_type);
	CuAssertIntEquals(ct, LOCAL_VAR, stmt->s_left->type);
	CuAssertIntEquals(ct, expected_index, stmt->s_left->local_var.index);
	CuAssertIntEquals(ct, expected_jvm_type,
			  stmt->s_left->local_var.type);
	CuAssertIntEquals(ct, stack_pop(&stack), stmt->s_target->temporary);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free_stmt(stmt);
}

void test_convert_iload(CuTest * ct)
{
	assert_load_stmt(ct, OPC_ILOAD, J_INT, 0x00);
	assert_load_stmt(ct, OPC_ILOAD, J_INT, 0x01);
	assert_load_stmt(ct, OPC_ILOAD, J_INT, 0xFF);
}

void test_convert_lload(CuTest * ct)
{
	assert_load_stmt(ct, OPC_LLOAD, J_LONG, 0x00);
	assert_load_stmt(ct, OPC_LLOAD, J_LONG, 0x01);
	assert_load_stmt(ct, OPC_LLOAD, J_LONG, 0xFF);
}

void test_convert_fload(CuTest * ct)
{
	assert_load_stmt(ct, OPC_FLOAD, J_FLOAT, 0x00);
	assert_load_stmt(ct, OPC_FLOAD, J_FLOAT, 0x01);
	assert_load_stmt(ct, OPC_FLOAD, J_FLOAT, 0xFF);
}

void test_convert_dload(CuTest * ct)
{
	assert_load_stmt(ct, OPC_DLOAD, J_DOUBLE, 0x00);
	assert_load_stmt(ct, OPC_DLOAD, J_DOUBLE, 0x01);
	assert_load_stmt(ct, OPC_DLOAD, J_DOUBLE, 0xFF);
}

void test_convert_aload(CuTest * ct)
{
	assert_load_stmt(ct, OPC_ALOAD, J_REFERENCE, 0x00);
	assert_load_stmt(ct, OPC_ALOAD, J_REFERENCE, 0x01);
	assert_load_stmt(ct, OPC_ALOAD, J_REFERENCE, 0xFF);
}

void test_convert_iload_n(CuTest * ct)
{
	assert_load_stmt(ct, OPC_ILOAD_0, J_INT, 0x00);
	assert_load_stmt(ct, OPC_ILOAD_1, J_INT, 0x01);
	assert_load_stmt(ct, OPC_ILOAD_2, J_INT, 0x02);
	assert_load_stmt(ct, OPC_ILOAD_3, J_INT, 0x03);
}

void test_convert_lload_n(CuTest * ct)
{
	assert_load_stmt(ct, OPC_LLOAD_0, J_LONG, 0x00);
	assert_load_stmt(ct, OPC_LLOAD_1, J_LONG, 0x01);
	assert_load_stmt(ct, OPC_LLOAD_2, J_LONG, 0x02);
	assert_load_stmt(ct, OPC_LLOAD_3, J_LONG, 0x03);
}

void test_convert_fload_n(CuTest * ct)
{
	assert_load_stmt(ct, OPC_FLOAD_0, J_FLOAT, 0x00);
	assert_load_stmt(ct, OPC_FLOAD_1, J_FLOAT, 0x01);
	assert_load_stmt(ct, OPC_FLOAD_2, J_FLOAT, 0x02);
	assert_load_stmt(ct, OPC_FLOAD_3, J_FLOAT, 0x03);
}

void test_convert_dload_n(CuTest * ct)
{
	assert_load_stmt(ct, OPC_DLOAD_0, J_DOUBLE, 0x00);
	assert_load_stmt(ct, OPC_DLOAD_1, J_DOUBLE, 0x01);
	assert_load_stmt(ct, OPC_DLOAD_2, J_DOUBLE, 0x02);
	assert_load_stmt(ct, OPC_DLOAD_3, J_DOUBLE, 0x03);
}

void test_convert_aload_n(CuTest * ct)
{
	assert_load_stmt(ct, OPC_ALOAD_0, J_REFERENCE, 0x00);
	assert_load_stmt(ct, OPC_ALOAD_1, J_REFERENCE, 0x01);
	assert_load_stmt(ct, OPC_ALOAD_2, J_REFERENCE, 0x02);
	assert_load_stmt(ct, OPC_ALOAD_3, J_REFERENCE, 0x03);
}

static void assert_null_check_stmt(CuTest *ct, unsigned long expected_ref,
				   struct statement *actual)
{
	CuAssertIntEquals(ct, STMT_NULL_CHECK, actual->s_type);
	assert_temporary_expression(ct, expected_ref, actual->s_left);
}

static void assert_arraycheck_stmt(CuTest *ct,
				   unsigned long expected_arrayref,
				   unsigned long expected_index,
				   struct statement *actual)
{
	CuAssertIntEquals(ct, STMT_ARRAY_CHECK, actual->s_type);
	assert_temporary_expression(ct, expected_arrayref, actual->s_left);
	assert_temporary_expression(ct, expected_index, actual->s_right);
}

static void assert_array_load_stmts(CuTest * ct, unsigned char opc,
				unsigned long arrayref, unsigned long index)
{
	unsigned char code[] = { opc };
	struct stack stack = STACK_INIT;
	stack_push(&stack, arrayref);
	stack_push(&stack, index);

	struct statement *stmt =
	    convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);

	struct statement *nullcheck = stmt;
	struct statement *arraycheck = stmt->s_next;
	struct statement *assign = arraycheck->s_next;

	assert_null_check_stmt(ct, arrayref, nullcheck);
	assert_arraycheck_stmt(ct, arrayref, index, arraycheck);
	CuAssertIntEquals(ct, STMT_ASSIGN, assign->s_type);
	assert_arrayref_expression(ct, arrayref, index, assign->s_left);

	CuAssertIntEquals(ct, stack_pop(&stack), assign->s_target->temporary);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));

	free_stmt(stmt);
}

void test_convert_iaload(CuTest * ct)
{
	assert_array_load_stmts(ct, OPC_IALOAD, 0, 1);
	assert_array_load_stmts(ct, OPC_IALOAD, 1, 2);
}

void test_convert_laload(CuTest * ct)
{
	assert_array_load_stmts(ct, OPC_LALOAD, 0, 1);
	assert_array_load_stmts(ct, OPC_LALOAD, 1, 2);
}

void test_convert_faload(CuTest * ct)
{
	assert_array_load_stmts(ct, OPC_FALOAD, 0, 1);
	assert_array_load_stmts(ct, OPC_FALOAD, 1, 2);
}

void test_convert_daload(CuTest * ct)
{
	assert_array_load_stmts(ct, OPC_DALOAD, 0, 1);
	assert_array_load_stmts(ct, OPC_DALOAD, 1, 2);
}

void test_convert_aaload(CuTest * ct)
{
	assert_array_load_stmts(ct, OPC_AALOAD, 0, 1);
	assert_array_load_stmts(ct, OPC_AALOAD, 1, 2);
}

void test_convert_baload(CuTest * ct)
{
	assert_array_load_stmts(ct, OPC_BALOAD, 0, 1);
	assert_array_load_stmts(ct, OPC_BALOAD, 1, 2);
}

void test_convert_caload(CuTest * ct)
{
	assert_array_load_stmts(ct, OPC_CALOAD, 0, 1);
	assert_array_load_stmts(ct, OPC_CALOAD, 1, 2);
}

void test_convert_saload(CuTest * ct)
{
	assert_array_load_stmts(ct, OPC_SALOAD, 0, 1);
	assert_array_load_stmts(ct, OPC_SALOAD, 1, 2);
}

static void assert_store_stmt(CuTest * ct, unsigned char opc,
			       enum jvm_type expected_jvm_type,
			       unsigned char expected_index,
			       unsigned long expected_temporary)
{
	unsigned char code[] = { opc, expected_index };
	struct stack stack = STACK_INIT;
	stack_push(&stack, expected_temporary);

	struct statement *stmt =
	    convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);

	CuAssertIntEquals(ct, STMT_ASSIGN, stmt->s_type);

	CuAssertIntEquals(ct, TEMPORARY, stmt->s_left->type);
	CuAssertIntEquals(ct, expected_temporary, stmt->s_left->temporary);

	CuAssertIntEquals(ct, LOCAL_VAR, stmt->s_target->type);
	CuAssertIntEquals(ct, expected_index, stmt->s_target->local_var.index);
	CuAssertIntEquals(ct, expected_jvm_type, stmt->s_target->local_var.type);

	CuAssertIntEquals(ct, true, stack_is_empty(&stack));

	free_stmt(stmt);
}

void test_convert_istore(CuTest * ct)
{
	assert_store_stmt(ct, OPC_ISTORE, J_INT, 0x00, 0x01);
	assert_store_stmt(ct, OPC_ISTORE, J_INT, 0x01, 0x02);
}

void test_convert_lstore(CuTest * ct)
{
	assert_store_stmt(ct, OPC_LSTORE, J_LONG, 0x00, 0x01);
	assert_store_stmt(ct, OPC_LSTORE, J_LONG, 0x01, 0x02);
}

void test_convert_fstore(CuTest * ct)
{
	assert_store_stmt(ct, OPC_FSTORE, J_FLOAT, 0x00, 0x01);
	assert_store_stmt(ct, OPC_FSTORE, J_FLOAT, 0x01, 0x02);
}

void test_convert_dstore(CuTest * ct)
{
	assert_store_stmt(ct, OPC_DSTORE, J_DOUBLE, 0x00, 0x01);
	assert_store_stmt(ct, OPC_DSTORE, J_DOUBLE, 0x01, 0x02);
}

void test_convert_astore(CuTest * ct)
{
	assert_store_stmt(ct, OPC_ASTORE, J_REFERENCE, 0x00, 0x01);
	assert_store_stmt(ct, OPC_ASTORE, J_REFERENCE, 0x01, 0x02);
}

void test_convert_istore_n(CuTest * ct)
{
	assert_store_stmt(ct, OPC_ISTORE_0, J_INT, 0x00, 0xFF);
	assert_store_stmt(ct, OPC_ISTORE_1, J_INT, 0x01, 0xFF);
	assert_store_stmt(ct, OPC_ISTORE_2, J_INT, 0x02, 0xFF);
	assert_store_stmt(ct, OPC_ISTORE_3, J_INT, 0x03, 0xFF);
}

void test_convert_lstore_n(CuTest * ct)
{
	assert_store_stmt(ct, OPC_LSTORE_0, J_LONG, 0x00, 0xFF);
	assert_store_stmt(ct, OPC_LSTORE_1, J_LONG, 0x01, 0xFF);
	assert_store_stmt(ct, OPC_LSTORE_2, J_LONG, 0x02, 0xFF);
	assert_store_stmt(ct, OPC_LSTORE_3, J_LONG, 0x03, 0xFF);
}

void test_convert_fstore_n(CuTest * ct)
{
	assert_store_stmt(ct, OPC_FSTORE_0, J_FLOAT, 0x00, 0xFF);
	assert_store_stmt(ct, OPC_FSTORE_1, J_FLOAT, 0x01, 0xFF);
	assert_store_stmt(ct, OPC_FSTORE_2, J_FLOAT, 0x02, 0xFF);
	assert_store_stmt(ct, OPC_FSTORE_3, J_FLOAT, 0x03, 0xFF);
}

void test_convert_dstore_n(CuTest * ct)
{
	assert_store_stmt(ct, OPC_DSTORE_0, J_DOUBLE, 0x00, 0xFF);
	assert_store_stmt(ct, OPC_DSTORE_1, J_DOUBLE, 0x01, 0xFF);
	assert_store_stmt(ct, OPC_DSTORE_2, J_DOUBLE, 0x02, 0xFF);
	assert_store_stmt(ct, OPC_DSTORE_3, J_DOUBLE, 0x03, 0xFF);
}

void test_convert_astore_n(CuTest * ct)
{
	assert_store_stmt(ct, OPC_ASTORE_0, J_REFERENCE, 0x00, 0xFF);
	assert_store_stmt(ct, OPC_ASTORE_1, J_REFERENCE, 0x01, 0xFF);
	assert_store_stmt(ct, OPC_ASTORE_2, J_REFERENCE, 0x02, 0xFF);
	assert_store_stmt(ct, OPC_ASTORE_3, J_REFERENCE, 0x03, 0xFF);
}

static void assert_array_store_stmts(CuTest * ct, unsigned char opc,
				     unsigned long arrayref,
				     unsigned long index,
				     unsigned long value)
{
	unsigned char code[] = { opc };
	struct stack stack = STACK_INIT;
	stack_push(&stack, arrayref);
	stack_push(&stack, index);
	stack_push(&stack, value);

	struct statement *stmt =
	    convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);

	struct statement *nullcheck = stmt;
	struct statement *arraycheck = stmt->s_next;
	struct statement *assign = arraycheck->s_next;

	assert_null_check_stmt(ct, arrayref, nullcheck);
	assert_arraycheck_stmt(ct, arrayref, index, arraycheck);
	CuAssertIntEquals(ct, STMT_ASSIGN, assign->s_type);
	assert_arrayref_expression(ct, arrayref, index, assign->s_target);
	CuAssertIntEquals(ct, value, assign->s_left->temporary);

	CuAssertIntEquals(ct, true, stack_is_empty(&stack));

	free_stmt(stmt);
}

void test_convert_iastore(CuTest * ct)
{
	assert_array_store_stmts(ct, OPC_IASTORE, 0, 1, 2);
	assert_array_store_stmts(ct, OPC_IASTORE, 2, 3, 4);
}

void test_convert_lastore(CuTest * ct)
{
	assert_array_store_stmts(ct, OPC_LASTORE, 0, 1, 2);
	assert_array_store_stmts(ct, OPC_LASTORE, 2, 3, 4);
}

void test_convert_fastore(CuTest * ct)
{
	assert_array_store_stmts(ct, OPC_FASTORE, 0, 1, 2);
	assert_array_store_stmts(ct, OPC_FASTORE, 2, 3, 4);
}

void test_convert_dastore(CuTest * ct)
{
	assert_array_store_stmts(ct, OPC_DASTORE, 0, 1, 2);
	assert_array_store_stmts(ct, OPC_DASTORE, 2, 3, 4);
}

void test_convert_aastore(CuTest * ct)
{
	assert_array_store_stmts(ct, OPC_AASTORE, 0, 1, 2);
	assert_array_store_stmts(ct, OPC_AASTORE, 2, 3, 4);
}

void test_convert_bastore(CuTest * ct)
{
	assert_array_store_stmts(ct, OPC_BASTORE, 0, 1, 2);
	assert_array_store_stmts(ct, OPC_BASTORE, 2, 3, 4);
}

void test_convert_castore(CuTest * ct)
{
	assert_array_store_stmts(ct, OPC_CASTORE, 0, 1, 2);
	assert_array_store_stmts(ct, OPC_CASTORE, 2, 3, 4);
}

void test_convert_sastore(CuTest * ct)
{
	assert_array_store_stmts(ct, OPC_SASTORE, 0, 1, 2);
	assert_array_store_stmts(ct, OPC_SASTORE, 2, 3, 4);
}

static void assert_pop_stack(CuTest * ct, unsigned char opc)
{
	unsigned char code[] = { opc };
	struct stack stack = STACK_INIT;
	stack_push(&stack, 1);
	convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
}

void test_convert_pop(CuTest * ct)
{
	assert_pop_stack(ct, OPC_POP);
	assert_pop_stack(ct, OPC_POP2);
}

static void assert_dup_stack(CuTest * ct, unsigned char opc, int expected)
{
	unsigned char code[] = { opc };
	struct stack stack = STACK_INIT;
	stack_push(&stack, expected);
	convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);
	CuAssertIntEquals(ct, stack_pop(&stack), expected);
	CuAssertIntEquals(ct, stack_pop(&stack), expected);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
}

void test_convert_dup(CuTest * ct)
{
	assert_dup_stack(ct, OPC_DUP, 1);
	assert_dup_stack(ct, OPC_DUP, 2);
	assert_dup_stack(ct, OPC_DUP2, 1);
	assert_dup_stack(ct, OPC_DUP2, 2);
}

static void assert_dup_x1_stack(CuTest * ct, unsigned char opc,
					int expected1, int expected2)
{
	unsigned char code[] = { opc };
	struct stack stack = STACK_INIT;
	stack_push(&stack, expected2);
	stack_push(&stack, expected1);
	convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);
	CuAssertIntEquals(ct, stack_pop(&stack), expected1);
	CuAssertIntEquals(ct, stack_pop(&stack), expected2);
	CuAssertIntEquals(ct, stack_pop(&stack), expected1);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
}

void test_convert_dup_x1(CuTest * ct)
{
	assert_dup_x1_stack(ct, OPC_DUP_X1, 1, 2);
	assert_dup_x1_stack(ct, OPC_DUP_X1, 2, 3);
	assert_dup_x1_stack(ct, OPC_DUP2_X1, 1, 2);
	assert_dup_x1_stack(ct, OPC_DUP2_X1, 2, 3);
}

static void assert_dup_x2_stack(CuTest * ct, unsigned char opc,
					int expected1, int expected2,
					int expected3)
{
	unsigned char code[] = { opc };
	struct stack stack = STACK_INIT;
	stack_push(&stack, expected3);
	stack_push(&stack, expected2);
	stack_push(&stack, expected1);
	convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);
	CuAssertIntEquals(ct, stack_pop(&stack), expected1);
	CuAssertIntEquals(ct, stack_pop(&stack), expected2);
	CuAssertIntEquals(ct, stack_pop(&stack), expected3);
	CuAssertIntEquals(ct, stack_pop(&stack), expected1);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
}

void test_convert_dup_x2(CuTest * ct)
{
	assert_dup_x2_stack(ct, OPC_DUP_X2, 1, 2, 3);
	assert_dup_x2_stack(ct, OPC_DUP_X2, 2, 3, 4);
	assert_dup_x2_stack(ct, OPC_DUP2_X2, 1, 2, 3);
	assert_dup_x2_stack(ct, OPC_DUP2_X2, 2, 3, 4);
}

static void assert_swap_stack(CuTest * ct, unsigned char opc,
				      int expected1, int expected2)
{
	unsigned char code[] = { opc };
	struct stack stack = STACK_INIT;
	stack_push(&stack, expected1);
	stack_push(&stack, expected2);

	convert_bytecode_to_stmts(NULL, code, sizeof(code), &stack);
	CuAssertIntEquals(ct, stack_pop(&stack), expected1);
	CuAssertIntEquals(ct, stack_pop(&stack), expected2);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
}

void test_convert_swap(CuTest * ct)
{
	assert_swap_stack(ct, OPC_SWAP, 1, 2);
	assert_swap_stack(ct, OPC_SWAP, 2, 3);
}
