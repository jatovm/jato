/*
 * Copyright (C) 2005  Pekka Enberg
 */

#include <statement.h>
#include <constant.h>
#include <byteorder.h>
#include <operand-stack.h>

#include <CuTest.h>
#include <stdlib.h>

void test_convert_nop(CuTest *ct)
{
	unsigned char code[] = { OPC_NOP };
	struct operand_stack stack = OPERAND_STACK_INIT;
	struct statement *stmt = stmt_from_bytecode(NULL, code, sizeof(code), &stack);
	CuAssertIntEquals(ct, STMT_NOP, stmt->type);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free(stmt);
}

static void assert_long_long_const_operand(CuTest *ct, struct statement *stmt,
					 enum constant_type expected_const_type,
					 long long expected_value)
{
	CuAssertIntEquals(ct, OPERAND_CONSTANT, stmt->s_left.o_type);
	CuAssertIntEquals(ct, expected_const_type, stmt->s_left.o_const.type);
	CuAssertIntEquals(ct, expected_value, stmt->s_left.o_const.value);
}

static void __assert_stmt_operand_long(CuTest *ct, struct classblock *cb,
				       enum statement_type expected_stmt_type,
				       enum constant_type expected_const_type,
				       long long expected_value,
				       char *actual, size_t count)
{
	struct operand_stack stack = OPERAND_STACK_INIT;
	struct statement *stmt = stmt_from_bytecode(cb, actual, count, NULL);
	CuAssertIntEquals(ct, expected_stmt_type, stmt->type);
	assert_long_long_const_operand(ct, stmt, expected_const_type, expected_value);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free(stmt);
}

static void assert_stmt_operand_long(CuTest *ct,
				     enum constant_type expected_const_type,
				     long long expected_value,
				     char actual)
{
	unsigned char code[] = { actual };
	__assert_stmt_operand_long(ct, NULL, STMT_ASSIGN,
				   expected_const_type, expected_value,
				   code, sizeof(code));
}

void test_convert_aconst_null(CuTest *ct)
{
	assert_stmt_operand_long(ct, CONST_REFERENCE, 0, OPC_ACONST_NULL);
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

static void assert_double_const_operand(CuTest *ct,
					   enum constant_type expected_const_type,
					   double expected_value,
					   struct operand *operand)
{
	CuAssertIntEquals(ct, OPERAND_CONSTANT, operand->o_type);
	CuAssertIntEquals(ct, expected_const_type, operand->o_const.type);
	CuAssertDblEquals(ct, expected_value, operand->o_const.fvalue, 0.01f);
}

static void assert_stmt_operand_double(CuTest *ct,
				       enum constant_type expected_const_type,
				       double expected_value,
				       char actual)
{
	unsigned char code[] = { actual };
	struct operand_stack stack = OPERAND_STACK_INIT;
	struct statement *stmt = stmt_from_bytecode(NULL, code, sizeof(code), &stack);
	CuAssertIntEquals(ct, STMT_ASSIGN, stmt->type);
	assert_double_const_operand(ct, expected_const_type, expected_value, &stmt->s_left);
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
				   long long expected_value,
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

static void assert_stmt_for_ldc_int(CuTest *ct, enum constant_type expected_const_type,
				    long expected_value, u1 cp_type)
{
	ConstantPoolEntry cp_infos[] = { cpu_to_be64(expected_value) };
	u1 cp_types[] = { cp_type };
	struct operand_stack stack = OPERAND_STACK_INIT;

	struct statement *stmt =
		create_stmt_with_constant_pool(cp_infos, sizeof(cp_infos),
					       cp_types, OPC_LDC, 0x00, 0x00,
					       &stack);
	CuAssertIntEquals(ct, STMT_ASSIGN, stmt->type);
	assert_long_long_const_operand(ct, stmt, expected_const_type, expected_value);
	CuAssertIntEquals(ct, stack_pop(&stack), stmt->target);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free(stmt);
}

static void assert_stmt_for_ldc_float(CuTest *ct, float expected_value)
{
	u4 value = *(u4*) &expected_value;
	ConstantPoolEntry cp_infos[] = { cpu_to_be64(value) };
	u1 cp_types[] = { CONSTANT_Float };
	struct operand_stack stack = OPERAND_STACK_INIT;

	struct statement *stmt =
		create_stmt_with_constant_pool(cp_infos, sizeof(cp_infos),
					       cp_types, OPC_LDC, 0x00, 0x00,
					       &stack);
	CuAssertIntEquals(ct, STMT_ASSIGN, stmt->type);
	assert_double_const_operand(ct, CONST_FLOAT, expected_value, &stmt->s_left);
	CuAssertIntEquals(ct, stack_pop(&stack), stmt->target);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free(stmt);
}

#define INT_MAX 2147483647
#define INT_MIN (-INT_MAX - 1)

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

static void assert_stmt_for_ldc_x_long(CuTest *ct, enum constant_type expected_const_type,
				       long long expected_value, u1 cp_type,
				       unsigned char opcode)
{
	ConstantPoolEntry cp_infos[257];
	cp_infos[256] = cpu_to_be64(expected_value);
	u1 cp_types[257];
	cp_types[256] = cp_type;
	struct operand_stack stack = OPERAND_STACK_INIT;

	struct statement *stmt =
		create_stmt_with_constant_pool(cp_infos, sizeof(cp_infos),
					       cp_types, opcode, 0x01, 0x00,
					       &stack);
	CuAssertIntEquals(ct, STMT_ASSIGN, stmt->type);
	assert_long_long_const_operand(ct, stmt, expected_const_type, expected_value);
	CuAssertIntEquals(ct, stack_pop(&stack), stmt->target);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free(stmt);
}

static void __assert_stmt_for_ldc_x_double(CuTest *ct,
					   enum constant_type expected_constant_type,
					   double expected_value, u1 cp_type,
					   u8 value, unsigned long opcode)
{
	ConstantPoolEntry cp_infos[257];
	cp_infos[256] = cpu_to_be64(value);
	u1 cp_types[257];
	cp_types[256] = cp_type;
	struct operand_stack stack = OPERAND_STACK_INIT;

	struct statement *stmt =
		create_stmt_with_constant_pool(cp_infos, sizeof(cp_infos),
					       cp_types, opcode, 0x01, 0x00,
					       &stack);
	CuAssertIntEquals(ct, STMT_ASSIGN, stmt->type);
	assert_double_const_operand(ct, expected_constant_type, expected_value, &stmt->s_left);
	CuAssertIntEquals(ct, stack_pop(&stack), stmt->target);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free(stmt);
}

static void assert_stmt_for_ldc_x_float(CuTest *ct,
					 enum constant_type expected_constant_type,
					 float expected_value, u1 cp_type,
					 unsigned long opcode)
{
	u4 value = *(u4*) &expected_value;
	__assert_stmt_for_ldc_x_double(ct, expected_constant_type,
				       expected_value, cp_type, value, opcode);
}

void test_convert_ldc_w(CuTest *ct)
{
	assert_stmt_for_ldc_x_long(ct, CONST_INT, 0, CONSTANT_Integer, OPC_LDC_W);
	assert_stmt_for_ldc_x_long(ct, CONST_INT, 1, CONSTANT_Integer, OPC_LDC_W);
	assert_stmt_for_ldc_x_long(ct, CONST_INT, INT_MIN, CONSTANT_Integer, OPC_LDC_W);
	assert_stmt_for_ldc_x_long(ct, CONST_INT, INT_MAX, CONSTANT_Integer, OPC_LDC_W);
	assert_stmt_for_ldc_x_float(ct, CONST_FLOAT, 0.01f, CONSTANT_Float, OPC_LDC_W);
	assert_stmt_for_ldc_x_float(ct, CONST_FLOAT, 1.0f, CONSTANT_Float, OPC_LDC_W);
	assert_stmt_for_ldc_x_float(ct, CONST_FLOAT, -1.0f, CONSTANT_Float, OPC_LDC_W);
	assert_stmt_for_ldc_x_long(ct, CONST_REFERENCE, 0xDEADBEEF, CONSTANT_String, OPC_LDC_W);
}

static void assert_stmt_for_ldc_x_double(CuTest *ct,
					  enum constant_type expected_constant_type,
					  double expected_value, u1 cp_type,
					  unsigned long opcode)
{
	u8 value = *(u8*) &expected_value;
	__assert_stmt_for_ldc_x_double(ct, expected_constant_type,
				       expected_value, cp_type, value, opcode);
}

#define LONG_MAX ((long long) 2<<63)
#define LONG_MIN (-LONG_MAX - 1)

void test_convert_ldc2_w(CuTest *ct)
{
	assert_stmt_for_ldc_x_long(ct, CONST_LONG, 0, CONSTANT_Long, OPC_LDC2_W);
	assert_stmt_for_ldc_x_long(ct, CONST_LONG, 1, CONSTANT_Long, OPC_LDC2_W);
	assert_stmt_for_ldc_x_long(ct, CONST_LONG, LONG_MIN, CONSTANT_Long, OPC_LDC2_W);
	assert_stmt_for_ldc_x_long(ct, CONST_LONG, LONG_MAX, CONSTANT_Long, OPC_LDC2_W);
	assert_stmt_for_ldc_x_double(ct, CONST_DOUBLE, 0.01f, CONSTANT_Double, OPC_LDC2_W);
	assert_stmt_for_ldc_x_double(ct, CONST_DOUBLE, 1.0f, CONSTANT_Double, OPC_LDC2_W);
	assert_stmt_for_ldc_x_double(ct, CONST_DOUBLE, -1.0f, CONSTANT_Double, OPC_LDC2_W);
}

static void assert_stmt_for_load(CuTest *ct, unsigned char opc,
				  enum local_variable_type expected_local_variable_type,
				  unsigned char expected_index)
{
	unsigned char code[] = { opc, expected_index };
	struct operand_stack stack = OPERAND_STACK_INIT;
	struct statement *stmt = stmt_from_bytecode(NULL, code, sizeof(code), &stack);
	CuAssertIntEquals(ct, STMT_ASSIGN, stmt->type);
	CuAssertIntEquals(ct, OPERAND_LOCAL_VARIABLE, stmt->s_left.o_type);
	CuAssertIntEquals(ct, expected_index, stmt->s_left.o_local.lv_index);
	CuAssertIntEquals(ct, expected_local_variable_type, stmt->s_left.o_local.lv_type);
	CuAssertIntEquals(ct, stack_pop(&stack), stmt->target);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
	free(stmt);
}

void test_convert_iload(CuTest *ct)
{
	assert_stmt_for_load(ct, OPC_ILOAD, LOCAL_VARIABLE_INT, 0x00);
	assert_stmt_for_load(ct, OPC_ILOAD, LOCAL_VARIABLE_INT, 0x01);
	assert_stmt_for_load(ct, OPC_ILOAD, LOCAL_VARIABLE_INT, 0xFF);
}

void test_convert_lload(CuTest *ct)
{
	assert_stmt_for_load(ct, OPC_LLOAD, LOCAL_VARIABLE_LONG, 0x00);
	assert_stmt_for_load(ct, OPC_LLOAD, LOCAL_VARIABLE_LONG, 0x01);
	assert_stmt_for_load(ct, OPC_LLOAD, LOCAL_VARIABLE_LONG, 0xFF);
}

void test_convert_fload(CuTest *ct)
{
	assert_stmt_for_load(ct, OPC_FLOAD, LOCAL_VARIABLE_FLOAT, 0x00);
	assert_stmt_for_load(ct, OPC_FLOAD, LOCAL_VARIABLE_FLOAT, 0x01);
	assert_stmt_for_load(ct, OPC_FLOAD, LOCAL_VARIABLE_FLOAT, 0xFF);
}

void test_convert_dload(CuTest *ct)
{
	assert_stmt_for_load(ct, OPC_DLOAD, LOCAL_VARIABLE_DOUBLE, 0x00);
	assert_stmt_for_load(ct, OPC_DLOAD, LOCAL_VARIABLE_DOUBLE, 0x01);
	assert_stmt_for_load(ct, OPC_DLOAD, LOCAL_VARIABLE_DOUBLE, 0xFF);
}

void test_convert_aload(CuTest *ct)
{
	assert_stmt_for_load(ct, OPC_ALOAD, LOCAL_VARIABLE_REFERENCE, 0x00);
	assert_stmt_for_load(ct, OPC_ALOAD, LOCAL_VARIABLE_REFERENCE, 0x01);
	assert_stmt_for_load(ct, OPC_ALOAD, LOCAL_VARIABLE_REFERENCE, 0xFF);
}

void test_convert_iload_x(CuTest *ct)
{
	assert_stmt_for_load(ct, OPC_ILOAD_0, LOCAL_VARIABLE_INT, 0x00);
	assert_stmt_for_load(ct, OPC_ILOAD_1, LOCAL_VARIABLE_INT, 0x01);
	assert_stmt_for_load(ct, OPC_ILOAD_2, LOCAL_VARIABLE_INT, 0x02);
	assert_stmt_for_load(ct, OPC_ILOAD_3, LOCAL_VARIABLE_INT, 0x03);
}

void test_convert_lload_x(CuTest *ct)
{
	assert_stmt_for_load(ct, OPC_LLOAD_0, LOCAL_VARIABLE_LONG, 0x00);
	assert_stmt_for_load(ct, OPC_LLOAD_1, LOCAL_VARIABLE_LONG, 0x01);
	assert_stmt_for_load(ct, OPC_LLOAD_2, LOCAL_VARIABLE_LONG, 0x02);
	assert_stmt_for_load(ct, OPC_LLOAD_3, LOCAL_VARIABLE_LONG, 0x03);
}

void test_convert_fload_x(CuTest *ct)
{
	assert_stmt_for_load(ct, OPC_FLOAD_0, LOCAL_VARIABLE_FLOAT, 0x00);
	assert_stmt_for_load(ct, OPC_FLOAD_1, LOCAL_VARIABLE_FLOAT, 0x01);
	assert_stmt_for_load(ct, OPC_FLOAD_2, LOCAL_VARIABLE_FLOAT, 0x02);
	assert_stmt_for_load(ct, OPC_FLOAD_3, LOCAL_VARIABLE_FLOAT, 0x03);
}

void test_convert_dload_x(CuTest *ct)
{
	assert_stmt_for_load(ct, OPC_DLOAD_0, LOCAL_VARIABLE_DOUBLE, 0x00);
	assert_stmt_for_load(ct, OPC_DLOAD_1, LOCAL_VARIABLE_DOUBLE, 0x01);
	assert_stmt_for_load(ct, OPC_DLOAD_2, LOCAL_VARIABLE_DOUBLE, 0x02);
	assert_stmt_for_load(ct, OPC_DLOAD_3, LOCAL_VARIABLE_DOUBLE, 0x03);
}

void test_convert_aload_x(CuTest *ct)
{
	assert_stmt_for_load(ct, OPC_ALOAD_0, LOCAL_VARIABLE_REFERENCE, 0x00);
	assert_stmt_for_load(ct, OPC_ALOAD_1, LOCAL_VARIABLE_REFERENCE, 0x01);
	assert_stmt_for_load(ct, OPC_ALOAD_2, LOCAL_VARIABLE_REFERENCE, 0x02);
	assert_stmt_for_load(ct, OPC_ALOAD_3, LOCAL_VARIABLE_REFERENCE, 0x03);
}

static void assert_temporary_operand(CuTest *ct, unsigned long expected,
				     struct operand *operand)
{
	CuAssertIntEquals(ct, OPERAND_TEMPORARY, operand->o_type);
	CuAssertIntEquals(ct, expected, operand->o_temporary);
}

static void assert_arrayref_operand(CuTest *ct, unsigned long expected_arrayref,
				    unsigned long expected_index,
				    struct operand *operand)
{
	CuAssertIntEquals(ct, OPERAND_ARRAYREF, operand->o_type);
	CuAssertIntEquals(ct, expected_arrayref, operand->o_arrayref);
	CuAssertIntEquals(ct, expected_index, operand->o_index);
}

static void assert_convert_x_aload(CuTest *ct, unsigned char opc,
				   unsigned long arrayref, unsigned long index)
{
	unsigned char code[] = { opc };
	struct operand_stack stack = OPERAND_STACK_INIT;
	stack_push(&stack, arrayref);
	stack_push(&stack, index);
	struct statement *stmt = stmt_from_bytecode(NULL, code, sizeof(code), &stack);

	struct statement *nullcheck = stmt;
	CuAssertIntEquals(ct, STMT_NULL_CHECK, nullcheck->type);
	assert_temporary_operand(ct, arrayref, &nullcheck->s_left);

	struct statement *arraycheck = stmt->next;
	CuAssertIntEquals(ct, STMT_ARRAY_CHECK, arraycheck->type);
	assert_temporary_operand(ct, arrayref, &arraycheck->s_left);
	assert_temporary_operand(ct, index, &arraycheck->s_right);

	struct statement *assign = arraycheck->next;
	CuAssertIntEquals(ct, STMT_ASSIGN, assign->type);
	assert_arrayref_operand(ct, arrayref, index, &assign->s_left);
	CuAssertIntEquals(ct, stack_pop(&stack), assign->target);
	CuAssertIntEquals(ct, true, stack_is_empty(&stack));
}

void test_convert_iaload(CuTest *ct)
{
	assert_convert_x_aload(ct, OPC_IALOAD, 0, 1);
	assert_convert_x_aload(ct, OPC_IALOAD, 1, 2);
}

void test_convert_laload(CuTest *ct)
{
	assert_convert_x_aload(ct, OPC_LALOAD, 0, 1);
	assert_convert_x_aload(ct, OPC_LALOAD, 1, 2);
}

void test_convert_faload(CuTest *ct)
{
	assert_convert_x_aload(ct, OPC_FALOAD, 0, 1);
	assert_convert_x_aload(ct, OPC_FALOAD, 1, 2);
}

void test_convert_daload(CuTest *ct)
{
	assert_convert_x_aload(ct, OPC_DALOAD, 0, 1);
	assert_convert_x_aload(ct, OPC_DALOAD, 1, 2);
}

void test_convert_aaload(CuTest *ct)
{
	assert_convert_x_aload(ct, OPC_AALOAD, 0, 1);
	assert_convert_x_aload(ct, OPC_AALOAD, 1, 2);
}
