/*
 * Copyright (C) 2005  Pekka Enberg
 */

#include <statement.h>
#include <constant.h>
#include <jam.h>

#include <CuTest.h>
#include <stdlib.h>

static void assert_stmt_type(CuTest *ct, enum statement_type expected, char actual)
{
	char code[] = { actual };
	struct statement *stmt = stmt_from_bytecode(code, sizeof(code));
	CuAssertIntEquals(ct, expected, stmt->type);
	free(stmt);
}

void test_convert_nop(CuTest *ct)
{
	assert_stmt_type(ct, STMT_NOP, OPC_NOP);
}

static void assert_stmt_const_operand(CuTest *ct,
				      enum statement_type expected_stmt_type,
				      enum constant_type expected_const_type,
				      char actual)
{
	char code[] = { actual };
	struct statement *stmt = stmt_from_bytecode(code, sizeof(code));
	CuAssertIntEquals(ct, expected_stmt_type, stmt->type);
	CuAssertIntEquals(ct, expected_const_type, stmt->operand.type);
	free(stmt);
}

void test_convert_aconst_null(CuTest *ct)
{
	assert_stmt_const_operand(ct, STMT_ASSIGN, CONST_NULL, OPC_ACONST_NULL);
}

static void __assert_stmt_operand_long(CuTest *ct,
				       enum statement_type expected_stmt_type,
				       enum constant_type expected_const_type,
				       unsigned long long expected_value,
				       char *actual, size_t count)
{
	struct statement *stmt = stmt_from_bytecode(actual, count);
	CuAssertIntEquals(ct, expected_stmt_type, stmt->type);
	CuAssertIntEquals(ct, expected_const_type, stmt->operand.type);
	CuAssertIntEquals(ct, expected_value, stmt->operand.value);
	free(stmt);
}

static void assert_stmt_operand_long(CuTest *ct,
				     enum constant_type expected_const_type,
				     unsigned long long expected_value,
				     char actual)
{
	char code[] = { actual };
	__assert_stmt_operand_long(ct, STMT_ASSIGN,
				   expected_const_type, expected_value,
				   code, sizeof(code));
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

static void assert_stmt_operand_double(CuTest *ct,
				       enum constant_type expected_const_type,
				       double expected_value,
				       char actual)
{
	char code[] = { actual };
	struct statement *stmt = stmt_from_bytecode(code, sizeof(code));
	CuAssertIntEquals(ct, STMT_ASSIGN, stmt->type);
	CuAssertIntEquals(ct, expected_const_type, stmt->operand.type);
	CuAssertDblEquals(ct, expected_value, stmt->operand.fvalue, 0.01f);
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
	char code[] = { actual, expected_value };
	__assert_stmt_operand_long(ct, STMT_ASSIGN, CONST_INT, expected_value,
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
	char code[] = { actual, first, second };
	__assert_stmt_operand_long(ct, STMT_ASSIGN, CONST_INT, expected_value,
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

static void assert_stmt_for_ldc(CuTest *ct, int expected_value)
{
	ConstantPoolEntry cp_infos[] = { expected_value };
	u1 cp_types[] = { CONSTANT_Integer };

	struct classblock cb = {
		.constant_pool_count = sizeof(cp_infos),
		.constant_pool.info = cp_infos,
		.constant_pool.type = cp_types
	};
	char code[] = { OPC_LDC_QUICK, 0x00 };
	__assert_stmt_operand_long(ct, STMT_ASSIGN, CONST_INT, expected_value,
				   code, sizeof(code));
}

void test_convert_ldc(CuTest *ct)
{
#if 0
	assert_stmt_for_ldc(ct, 0x0001);
#endif
}
