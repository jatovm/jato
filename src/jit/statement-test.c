/*
 * Copyright (C) 2005  Pekka Enberg
 */

#include <statement.h>
#include <constant.h>
#include <byteorder.h>
#include <operand-stack.h>

#include <CuTest.h>
#include <stdlib.h>

static void assert_stmt_type(CuTest *ct, enum statement_type expected, char actual)
{
	unsigned char code[] = { actual };
	struct operand_stack stack = OPERAND_STACK_INIT;
	struct statement *stmt = stmt_from_bytecode(NULL, code, sizeof(code), &stack);
	CuAssertIntEquals(ct, expected, stmt->type);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free(stmt);
}

void test_convert_nop(CuTest *ct)
{
	assert_stmt_type(ct, STMT_NOP, OPC_NOP);
}

static void assert_stmt_type_and_operand(CuTest *ct, struct statement *stmt,
					 enum statement_type expected_stmt_type,
					 enum constant_type expected_const_type,
					 unsigned long long expected_value)
{
	CuAssertIntEquals(ct, expected_stmt_type, stmt->type);
	CuAssertIntEquals(ct, expected_const_type, stmt->operand.type);
	CuAssertIntEquals(ct, expected_value, stmt->operand.value);
}

static void __assert_stmt_operand_long(CuTest *ct, struct classblock *cb,
				       enum statement_type expected_stmt_type,
				       enum constant_type expected_const_type,
				       unsigned long long expected_value,
				       char *actual, size_t count)
{
	struct operand_stack stack = OPERAND_STACK_INIT;
	struct statement *stmt = stmt_from_bytecode(cb, actual, count, NULL);
	assert_stmt_type_and_operand(ct, stmt, expected_stmt_type, expected_const_type, expected_value);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free(stmt);
}

static void assert_stmt_operand_long(CuTest *ct,
				     enum constant_type expected_const_type,
				     unsigned long long expected_value,
				     char actual)
{
	unsigned char code[] = { actual };
	__assert_stmt_operand_long(ct, NULL, STMT_ASSIGN,
				   expected_const_type, expected_value,
				   code, sizeof(code));
}

void test_convert_aconst_null(CuTest *ct)
{
	assert_stmt_operand_long(ct, CONST_NULL, 0, OPC_ACONST_NULL);
}

void test_convert_iconst(CuTest *ct)
{
	assert_stmt_operand_long(ct, CONST_INT, -1, OPC_ICONST_M1);
	assert_stmt_operand_long(ct, CONST_INT,  0, OPC_ICONST_0);
	assert_stmt_operand_long(ct, CONST_INT,  1, OPC_ICONST_1);
	assert_stmt_operand_long(ct, CONST_INT,  2, OPC_ICONST_2);
	assert_stmt_operand_long(ct, CONST_INT,  3, OPC_ICONST_3);
	assert_stmt_operand_long(ct, CONST_INT,  4, OPC_ICONST_4);
	assert_stmt_operand_long(ct, CONST_INT,  5, OPC_ICONST_5);
}

void test_convert_lconst(CuTest *ct)
{
	assert_stmt_operand_long(ct, CONST_LONG, 0, OPC_LCONST_0);
	assert_stmt_operand_long(ct, CONST_LONG, 1, OPC_LCONST_1);
}

static void __assert_stmt_operand_double(CuTest *ct,
					 struct statement *stmt,
				         enum constant_type expected_const_type,
				         double expected_value)
{
	CuAssertIntEquals(ct, STMT_ASSIGN, stmt->type);
	CuAssertIntEquals(ct, expected_const_type, stmt->operand.type);
	CuAssertDblEquals(ct, expected_value, stmt->operand.fvalue, 0.01f);
}

static void assert_stmt_operand_double(CuTest *ct,
				       enum constant_type expected_const_type,
				       double expected_value,
				       char actual)
{
	unsigned char code[] = { actual };
	struct operand_stack stack = OPERAND_STACK_INIT;
	struct statement *stmt = stmt_from_bytecode(NULL, code, sizeof(code), &stack);
	__assert_stmt_operand_double(ct, stmt, expected_const_type, expected_value);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free(stmt);
}

void test_convert_fconst(CuTest *ct)
{
	assert_stmt_operand_double(ct, CONST_FLOAT, 0, OPC_FCONST_0);
	assert_stmt_operand_double(ct, CONST_FLOAT, 1, OPC_FCONST_1);
	assert_stmt_operand_double(ct, CONST_FLOAT, 2, OPC_FCONST_2);
}

void test_convert_dconst(CuTest *ct)
{
	assert_stmt_operand_double(ct, CONST_DOUBLE, 0, OPC_DCONST_0);
	assert_stmt_operand_double(ct, CONST_DOUBLE, 1, OPC_DCONST_1);
}

static void assert_stmt_for_bipush(CuTest *ct,
				   char expected_value,
				   char actual)
{
	unsigned char code[] = { actual, expected_value };
	__assert_stmt_operand_long(ct, NULL, STMT_ASSIGN, CONST_INT, expected_value,
				   code, sizeof(code));
}

void test_convert_bipush(CuTest *ct)
{
	assert_stmt_for_bipush(ct, 0x0, OPC_BIPUSH);
	assert_stmt_for_bipush(ct, 0x1, OPC_BIPUSH);
	assert_stmt_for_bipush(ct, 0xFF, OPC_BIPUSH);
}

static void assert_stmt_for_sipush(CuTest *ct,
				   unsigned long long expected_value,
				   char first,
				   char second,
				   char actual)
{
	unsigned char code[] = { actual, first, second };
	__assert_stmt_operand_long(ct, NULL, STMT_ASSIGN, CONST_INT, expected_value,
				   code, sizeof(code));
}

#define MIN_SHORT (-32768)
#define MAX_SHORT 32767

void test_convert_sipush(CuTest *ct)
{
	assert_stmt_for_sipush(ct, 0, 0x00, 0x00, OPC_SIPUSH);
	assert_stmt_for_sipush(ct, 1, 0x00, 0x01, OPC_SIPUSH);
	assert_stmt_for_sipush(ct, MIN_SHORT, 0x80, 0x00, OPC_SIPUSH);
	assert_stmt_for_sipush(ct, MAX_SHORT, 0x7F, 0xFF, OPC_SIPUSH);
}

static struct statement *create_stmt_with_constant_pool(ConstantPoolEntry *cp_infos,
					     size_t nr_cp_infos, u1 *cp_types,
					     unsigned char opcode,
					     unsigned char index1,
					     unsigned char index2,
					     struct operand_stack *stack)
{
	struct classblock cb = {
		.constant_pool_count = sizeof(cp_infos),
		.constant_pool.info = cp_infos,
		.constant_pool.type = cp_types
	};
	unsigned char code[] = { opcode, index1, index2 };
	return stmt_from_bytecode(&cb, code, sizeof(code), stack);
}

static void assert_stmt_for_ldc_int(CuTest *ct, enum constant_type expected_const_type, long expected_value, u1 cp_type)
{
	ConstantPoolEntry cp_infos[] = { cpu_to_be32(expected_value) };
	u1 cp_types[] = { cp_type };
	struct operand_stack stack = OPERAND_STACK_INIT;

	struct statement *stmt = create_stmt_with_constant_pool(cp_infos, sizeof(cp_infos), cp_types, OPC_LDC, 0x00, 0x00, &stack);
	assert_stmt_type_and_operand(ct, stmt, STMT_ASSIGN, expected_const_type, expected_value);
	CuAssertIntEquals(ct, stack_pop(&stack), stmt->target);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free(stmt);
}

static void assert_stmt_for_ldc_float(CuTest *ct, float expected_value)
{
	u4 value = *(u4*) &expected_value;
	ConstantPoolEntry cp_infos[] = { cpu_to_be32(value) };
	u1 cp_types[] = { CONSTANT_Float };
	struct operand_stack stack = OPERAND_STACK_INIT;

	struct statement *stmt = create_stmt_with_constant_pool(cp_infos, sizeof(cp_infos), cp_types, OPC_LDC, 0x00, 0x00, &stack);
	__assert_stmt_operand_double(ct, stmt, CONST_FLOAT, expected_value);
	CuAssertIntEquals(ct, stack_pop(&stack), stmt->target);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free(stmt);
}

#define INT_MIN (-INT_MAX - 1)
#define INT_MAX 2147483647

void test_convert_ldc(CuTest *ct)
{
	assert_stmt_for_ldc_int(ct, CONST_INT, 0, CONSTANT_Integer);
	assert_stmt_for_ldc_int(ct, CONST_INT, 1, CONSTANT_Integer);
	assert_stmt_for_ldc_int(ct, CONST_INT, INT_MIN, CONSTANT_Integer);
	assert_stmt_for_ldc_int(ct, CONST_INT, INT_MAX, CONSTANT_Integer);
	assert_stmt_for_ldc_float(ct, 0.01f);
	assert_stmt_for_ldc_float(ct, 1.0f);
	assert_stmt_for_ldc_float(ct, -1.0f);
	assert_stmt_for_ldc_int(ct, CONST_REFERENCE, 0xDEADBEEF, CONSTANT_String);
}

static void assert_stmt_for_ldc_w_int(CuTest *ct, enum constant_type expected_const_type, long expected_value, u1 cp_type)
{
	ConstantPoolEntry cp_infos[257];
	cp_infos[256] = cpu_to_be32(expected_value);
	u1 cp_types[257];
	cp_types[256] = cp_type;
	struct operand_stack stack = OPERAND_STACK_INIT;

	struct statement *stmt = create_stmt_with_constant_pool(cp_infos, sizeof(cp_infos), cp_types, OPC_LDC_W, 0x01, 0x00, &stack);
	assert_stmt_type_and_operand(ct, stmt, STMT_ASSIGN, expected_const_type, expected_value);
	CuAssertIntEquals(ct, stack_pop(&stack), stmt->target);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free(stmt);
}

static void assert_stmt_for_ldc_w_float(CuTest *ct, float expected_value)
{
	u4 value = *(u4*) &expected_value;
	ConstantPoolEntry cp_infos[257];
	cp_infos[256] = cpu_to_be32(value);
	u1 cp_types[257];
	cp_types[256] = CONSTANT_Float;
	struct operand_stack stack = OPERAND_STACK_INIT;

	struct statement *stmt = create_stmt_with_constant_pool(cp_infos, sizeof(cp_infos), cp_types, OPC_LDC_W, 0x01, 0x00, &stack);
	__assert_stmt_operand_double(ct, stmt, CONST_FLOAT, expected_value);
	CuAssertIntEquals(ct, stack_pop(&stack), stmt->target);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free(stmt);
}

void test_convert_ldc_w(CuTest *ct)
{
	assert_stmt_for_ldc_w_int(ct, CONST_INT, 0, CONSTANT_Integer);
	assert_stmt_for_ldc_w_int(ct, CONST_INT, 1, CONSTANT_Integer);
	assert_stmt_for_ldc_w_int(ct, CONST_INT, INT_MIN, CONSTANT_Integer);
	assert_stmt_for_ldc_w_int(ct, CONST_INT, INT_MAX, CONSTANT_Integer);
	assert_stmt_for_ldc_w_float(ct, 0.01f);
	assert_stmt_for_ldc_w_float(ct, 1.0f);
	assert_stmt_for_ldc_w_float(ct, -1.0f);
	assert_stmt_for_ldc_w_int(ct, CONST_REFERENCE, 0xDEADBEEF, CONSTANT_String);
}
