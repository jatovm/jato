/*
 * Copyright (C) 2005  Pekka Enberg
 */

#include <jit-compiler.h>
#include <jit/bytecode-converters.h>
#include <jit/statement.h>
#include <vm/list.h>
#include <vm/stack.h>
#include <vm/system.h>

#include <bytecode-converter-fixture.h>
#include <libharness.h>
#include <stdlib.h>

static u4 float_to_cpu32(float fvalue)
{
	union {
		float fvalue;
		u4 value;
	} v;
	v.fvalue = fvalue;
	return v.value;
}

static u8 double_to_cpu64(double fvalue)
{
	union {
		double fvalue;
		u8 value;
	} v;
	v.fvalue = fvalue;
	return v.value;
}

static void __assert_convert_const(enum jvm_type expected_jvm_type,
				   long long expected_value,
				   struct methodblock *method)
{
	struct expression *expr;
	struct compilation_unit *cu;

	cu = alloc_simple_compilation_unit(method);
	convert_to_ir(cu);

	expr = stack_pop(cu->expr_stack);
	assert_value_expr(expected_jvm_type, expected_value, &expr->node);
	assert_true(stack_is_empty(cu->expr_stack));

	expr_put(expr);
	free_compilation_unit(cu);
}

static void assert_convert_const(enum jvm_type expected_jvm_type,
				 long long expected_value, char actual)
{
	unsigned char code[] = { actual };
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};
	__assert_convert_const(expected_jvm_type, expected_value, &method);
}

static void assert_convert_fconst(enum jvm_type expected_jvm_type,
				  double expected_value, char actual)
{
	struct expression *expr;
	unsigned char code[] = { actual };
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct compilation_unit *cu;

	cu = alloc_simple_compilation_unit(&method);
	convert_to_ir(cu);
	expr = stack_pop(cu->expr_stack);
	assert_fvalue_expr(expected_jvm_type, expected_value, &expr->node);
	assert_true(stack_is_empty(cu->expr_stack));

	expr_put(expr);
	free_compilation_unit(cu);
}

void test_convert_nop(void)
{
	unsigned char code[] = { OPC_NOP };
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct compilation_unit *cu;
	struct statement *stmt;

	cu = alloc_simple_compilation_unit(&method);

	convert_to_ir(cu);
	stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);
	assert_int_equals(STMT_NOP, stmt_type(stmt));
	assert_true(stack_is_empty(cu->expr_stack));

	free_compilation_unit(cu);
}

void test_convert_aconst_null(void)
{
	assert_convert_const(J_REFERENCE, 0, OPC_ACONST_NULL);
}

void test_convert_iconst(void)
{
	assert_convert_const(J_INT, -1, OPC_ICONST_M1);
	assert_convert_const(J_INT, 0, OPC_ICONST_0);
	assert_convert_const(J_INT, 1, OPC_ICONST_1);
	assert_convert_const(J_INT, 2, OPC_ICONST_2);
	assert_convert_const(J_INT, 3, OPC_ICONST_3);
	assert_convert_const(J_INT, 4, OPC_ICONST_4);
	assert_convert_const(J_INT, 5, OPC_ICONST_5);
}

void test_convert_lconst(void)
{
	assert_convert_const(J_LONG, 0, OPC_LCONST_0);
	assert_convert_const(J_LONG, 1, OPC_LCONST_1);
}

void test_convert_fconst(void)
{
	assert_convert_fconst(J_FLOAT, 0, OPC_FCONST_0);
	assert_convert_fconst(J_FLOAT, 1, OPC_FCONST_1);
	assert_convert_fconst(J_FLOAT, 2, OPC_FCONST_2);
}

void test_convert_dconst(void)
{
	assert_convert_fconst(J_DOUBLE, 0, OPC_DCONST_0);
	assert_convert_fconst(J_DOUBLE, 1, OPC_DCONST_1);
}

static void assert_convert_bipush(char expected_value, char actual)
{
	unsigned char code[] = { actual, expected_value };
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};
	__assert_convert_const(J_INT, expected_value, &method);
}

void test_convert_bipush(void)
{
	assert_convert_bipush(0x00, OPC_BIPUSH);
	assert_convert_bipush(0x01, OPC_BIPUSH);
	assert_convert_bipush(0xFF, OPC_BIPUSH);
}

static void assert_convert_sipush(long long expected_value,
				  char first, char second, char actual)
{
	unsigned char code[] = { actual, first, second };
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};
	__assert_convert_const(J_INT, expected_value, &method);
}

#define MIN_SHORT (-32768)
#define MAX_SHORT 32767

void test_convert_sipush(void)
{
	assert_convert_sipush(0, 0x00, 0x00, OPC_SIPUSH);
	assert_convert_sipush(1, 0x00, 0x01, OPC_SIPUSH);
	assert_convert_sipush(MIN_SHORT, 0x80, 0x00, OPC_SIPUSH);
	assert_convert_sipush(MAX_SHORT, 0x7F, 0xFF, OPC_SIPUSH);
}

static void assert_convert_ldc(enum jvm_type expected_jvm_type,
			       long long expected_value, u1 cp_type)
{
	struct expression *expr;
	u8 cp_infos[] = { expected_value };
	u1 cp_types[] = { cp_type };
	unsigned char code[] = { OPC_LDC, 0x00, 0x00 };
	struct compilation_unit *cu;
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};

	cu = alloc_simple_compilation_unit(&method);

	convert_ir_const(cu, (void *)cp_infos, 8, cp_types);
	expr = stack_pop(cu->expr_stack);
	assert_value_expr(expected_jvm_type, expected_value, &expr->node);
	assert_true(stack_is_empty(cu->expr_stack));

	expr_put(expr);
	free_compilation_unit(cu);
}

static void assert_convert_ldc_f(float expected_value)
{
	struct expression *expr;
	u8 cp_infos[] = { float_to_cpu32(expected_value) };
	u1 cp_types[] = { CONSTANT_Float };
	unsigned char code[] = { OPC_LDC, 0x00, 0x00 };
	struct compilation_unit *cu;
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};

	cu = alloc_simple_compilation_unit(&method);

	convert_ir_const(cu, (void *)cp_infos, 8, cp_types);
	expr = stack_pop(cu->expr_stack);
	assert_fvalue_expr(J_FLOAT, expected_value, &expr->node);
	assert_true(stack_is_empty(cu->expr_stack));

	expr_put(expr);
	free_compilation_unit(cu);
}

#define INT_MAX 2147483647
#define INT_MIN (-INT_MAX - 1)

void test_convert_ldc(void)
{
	assert_convert_ldc(J_INT, 0, CONSTANT_Integer);
	assert_convert_ldc(J_INT, 1, CONSTANT_Integer);
	assert_convert_ldc(J_INT, INT_MIN, CONSTANT_Integer);
	assert_convert_ldc(J_INT, INT_MAX, CONSTANT_Integer);
	assert_convert_ldc_f(0.01f);
	assert_convert_ldc_f(1.0f);
	assert_convert_ldc_f(-1.0f);
	assert_convert_ldc(J_REFERENCE, 0xDEADBEEF, CONSTANT_String);
}

static void assert_convert_ldcw(enum jvm_type expected_jvm_type,
				long long expected_value, u1 cp_type,
				unsigned char opcode)
{
	struct expression *expr;
	u8 cp_infos[129];
	cp_infos[128] = expected_value;
	u1 cp_types[257];
	cp_types[256] = cp_type;
	unsigned char code[] = { opcode, 0x01, 0x00 };
	struct compilation_unit *cu;
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};

	cu = alloc_simple_compilation_unit(&method);

	convert_ir_const(cu, (void *)cp_infos, 256, cp_types);
	expr = stack_pop(cu->expr_stack);
	assert_value_expr(expected_jvm_type, expected_value, &expr->node);
	assert_true(stack_is_empty(cu->expr_stack));

	expr_put(expr);
	free_compilation_unit(cu);
}

static void assert_convert_ldcw_f(enum jvm_type expected_jvm_type,
				  double expected_value,
				  u1 cp_type, u8 value, unsigned long opcode)
{
	u8 cp_infos[129];
	cp_infos[128] = value;
	u1 cp_types[257];
	cp_types[256] = cp_type;
	unsigned char code[] = { opcode, 0x01, 0x00 };
	struct expression *expr;
	struct compilation_unit *cu;
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};

	cu = alloc_simple_compilation_unit(&method);

	convert_ir_const(cu, (void *)cp_infos, 256, cp_types);
	expr = stack_pop(cu->expr_stack);
	assert_fvalue_expr(expected_jvm_type, expected_value, &expr->node);
	assert_true(stack_is_empty(cu->expr_stack));

	expr_put(expr);
	free_compilation_unit(cu);
}

static void assert_ldcw_float_expr_and_stack(enum jvm_type expected_jvm_type,
					     float expected_value, u1 cp_type,
					     unsigned long opcode)
{
	u4 value = float_to_cpu32(expected_value);
	assert_convert_ldcw_f(expected_jvm_type,
			      expected_value, cp_type, value, opcode);
}

static void assert_ldcw_double_expr_and_stack(enum jvm_type
					      expected_jvm_type,
					      double expected_value,
					      u1 cp_type, unsigned long opcode)
{
	u8 value = double_to_cpu64(expected_value);
	assert_convert_ldcw_f(expected_jvm_type,
			      expected_value, cp_type, value, opcode);
}

void test_convert_ldc_w(void)
{
	assert_convert_ldcw(J_INT, 0, CONSTANT_Integer, OPC_LDC_W);
	assert_convert_ldcw(J_INT, 1, CONSTANT_Integer, OPC_LDC_W);
	assert_convert_ldcw(J_INT, INT_MIN, CONSTANT_Integer, OPC_LDC_W);
	assert_convert_ldcw(J_INT, INT_MAX, CONSTANT_Integer, OPC_LDC_W);
	assert_ldcw_float_expr_and_stack(J_FLOAT, 0.01f, CONSTANT_Float,
					 OPC_LDC_W);
	assert_ldcw_float_expr_and_stack(J_FLOAT, 1.0f, CONSTANT_Float,
					 OPC_LDC_W);
	assert_ldcw_float_expr_and_stack(J_FLOAT, -1.0f, CONSTANT_Float,
					 OPC_LDC_W);
	assert_convert_ldcw(J_REFERENCE, 0xDEADBEEF, CONSTANT_String,
			    OPC_LDC_W);
}

#define LONG_MAX ((long long) 2<<63)
#define LONG_MIN (-LONG_MAX - 1)

void test_convert_ldc2_w(void)
{
	assert_convert_ldcw(J_LONG, 0, CONSTANT_Long, OPC_LDC2_W);
	assert_convert_ldcw(J_LONG, 1, CONSTANT_Long, OPC_LDC2_W);
	assert_convert_ldcw(J_LONG, LONG_MIN, CONSTANT_Long, OPC_LDC2_W);
	assert_convert_ldcw(J_LONG, LONG_MAX, CONSTANT_Long, OPC_LDC2_W);
	assert_ldcw_double_expr_and_stack(J_DOUBLE, 0.01f, CONSTANT_Double,
					  OPC_LDC2_W);
	assert_ldcw_double_expr_and_stack(J_DOUBLE, 1.0f, CONSTANT_Double,
					  OPC_LDC2_W);
	assert_ldcw_double_expr_and_stack(J_DOUBLE, -1.0f, CONSTANT_Double,
					  OPC_LDC2_W);
}

static void __assert_convert_load(unsigned char *code,
				  unsigned long size,
				  enum jvm_type expected_jvm_type,
				  unsigned char expected_index)
{
	struct expression *expr;
	struct compilation_unit *cu;
	struct methodblock method = {
		.code = code,
		.code_size = size,
	};

	cu = alloc_simple_compilation_unit(&method);

	convert_to_ir(cu);

	expr = stack_pop(cu->expr_stack);
	assert_local_expr(expected_jvm_type, expected_index, &expr->node);
	assert_true(stack_is_empty(cu->expr_stack));

	expr_put(expr);
	free_compilation_unit(cu);
}

static void assert_convert_load(unsigned char opc,
				enum jvm_type expected_jvm_type,
				unsigned char expected_index)
{
	unsigned char code[] = { opc, expected_index };
	__assert_convert_load(code, ARRAY_SIZE(code), expected_jvm_type,
			      expected_index);
}

void test_convert_iload(void)
{
	assert_convert_load(OPC_ILOAD, J_INT, 0x00);
	assert_convert_load(OPC_ILOAD, J_INT, 0x01);
	assert_convert_load(OPC_ILOAD, J_INT, 0xFF);
}

void test_convert_lload(void)
{
	assert_convert_load(OPC_LLOAD, J_LONG, 0x00);
	assert_convert_load(OPC_LLOAD, J_LONG, 0x01);
	assert_convert_load(OPC_LLOAD, J_LONG, 0xFF);
}

void test_convert_fload(void)
{
	assert_convert_load(OPC_FLOAD, J_FLOAT, 0x00);
	assert_convert_load(OPC_FLOAD, J_FLOAT, 0x01);
	assert_convert_load(OPC_FLOAD, J_FLOAT, 0xFF);
}

void test_convert_dload(void)
{
	assert_convert_load(OPC_DLOAD, J_DOUBLE, 0x00);
	assert_convert_load(OPC_DLOAD, J_DOUBLE, 0x01);
	assert_convert_load(OPC_DLOAD, J_DOUBLE, 0xFF);
}

void test_convert_aload(void)
{
	assert_convert_load(OPC_ALOAD, J_REFERENCE, 0x00);
	assert_convert_load(OPC_ALOAD, J_REFERENCE, 0x01);
	assert_convert_load(OPC_ALOAD, J_REFERENCE, 0xFF);
}

static void assert_convert_load_n(unsigned char opc,
				  enum jvm_type expected_jvm_type,
				  unsigned char expected_index)
{
	unsigned char code[] = { opc };
	__assert_convert_load(code, ARRAY_SIZE(code), expected_jvm_type,
			      expected_index);
}

void test_convert_iload_n(void)
{
	assert_convert_load_n(OPC_ILOAD_0, J_INT, 0x00);
	assert_convert_load_n(OPC_ILOAD_1, J_INT, 0x01);
	assert_convert_load_n(OPC_ILOAD_2, J_INT, 0x02);
	assert_convert_load_n(OPC_ILOAD_3, J_INT, 0x03);
}

void test_convert_lload_n(void)
{
	assert_convert_load_n(OPC_LLOAD_0, J_LONG, 0x00);
	assert_convert_load_n(OPC_LLOAD_1, J_LONG, 0x01);
	assert_convert_load_n(OPC_LLOAD_2, J_LONG, 0x02);
	assert_convert_load_n(OPC_LLOAD_3, J_LONG, 0x03);
}

void test_convert_fload_n(void)
{
	assert_convert_load_n(OPC_FLOAD_0, J_FLOAT, 0x00);
	assert_convert_load_n(OPC_FLOAD_1, J_FLOAT, 0x01);
	assert_convert_load_n(OPC_FLOAD_2, J_FLOAT, 0x02);
	assert_convert_load_n(OPC_FLOAD_3, J_FLOAT, 0x03);
}

void test_convert_dload_n(void)
{
	assert_convert_load_n(OPC_DLOAD_0, J_DOUBLE, 0x00);
	assert_convert_load_n(OPC_DLOAD_1, J_DOUBLE, 0x01);
	assert_convert_load_n(OPC_DLOAD_2, J_DOUBLE, 0x02);
	assert_convert_load_n(OPC_DLOAD_3, J_DOUBLE, 0x03);
}

void test_convert_aload_n(void)
{
	assert_convert_load_n(OPC_ALOAD_0, J_REFERENCE, 0x00);
	assert_convert_load_n(OPC_ALOAD_1, J_REFERENCE, 0x01);
	assert_convert_load_n(OPC_ALOAD_2, J_REFERENCE, 0x02);
	assert_convert_load_n(OPC_ALOAD_3, J_REFERENCE, 0x03);
}

static void assert_convert_array_load(enum jvm_type expected_type,
				      unsigned char opc,
				      unsigned long arrayref,
				      unsigned long index)
{
	unsigned char code[] = { opc };
	struct expression *arrayref_expr, *index_expr, *temporary_expr;
	struct statement *stmt;
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct compilation_unit *cu;
	unsigned long expected_temporary;

	cu = alloc_simple_compilation_unit(&method);

	arrayref_expr = value_expr(J_REFERENCE, arrayref);
	index_expr = value_expr(J_INT, index);

	stack_push(cu->expr_stack, arrayref_expr);
	stack_push(cu->expr_stack, index_expr);

	convert_to_ir(cu);
	stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);

	struct statement *nullcheck = stmt;
	struct statement *arraycheck = stmt_entry(nullcheck->stmt_list_node.next);
	struct statement *store_stmt = stmt_entry(arraycheck->stmt_list_node.next);

	assert_null_check_stmt(arrayref_expr, nullcheck);
	assert_arraycheck_stmt(expected_type, arrayref_expr, index_expr, arraycheck);

	assert_store_stmt(store_stmt);
	assert_array_deref_expr(expected_type, arrayref_expr, index_expr,
				store_stmt->store_src);

	temporary_expr = stack_pop(cu->expr_stack);

	expected_temporary = to_expr(store_stmt->store_dest)->temporary;
	assert_temporary_expr(expected_temporary, &temporary_expr->node);
	expr_put(temporary_expr);
	assert_true(stack_is_empty(cu->expr_stack));

	free_compilation_unit(cu);
}

void test_convert_iaload(void)
{
	assert_convert_array_load(J_INT, OPC_IALOAD, 0, 1);
	assert_convert_array_load(J_INT, OPC_IALOAD, 1, 2);
}

void test_convert_laload(void)
{
	assert_convert_array_load(J_LONG, OPC_LALOAD, 0, 1);
	assert_convert_array_load(J_LONG, OPC_LALOAD, 1, 2);
}

void test_convert_faload(void)
{
	assert_convert_array_load(J_FLOAT, OPC_FALOAD, 0, 1);
	assert_convert_array_load(J_FLOAT, OPC_FALOAD, 1, 2);
}

void test_convert_daload(void)
{
	assert_convert_array_load(J_DOUBLE, OPC_DALOAD, 0, 1);
	assert_convert_array_load(J_DOUBLE, OPC_DALOAD, 1, 2);
}

void test_convert_aaload(void)
{
	assert_convert_array_load(J_REFERENCE, OPC_AALOAD, 0, 1);
	assert_convert_array_load(J_REFERENCE, OPC_AALOAD, 1, 2);
}

void test_convert_baload(void)
{
	assert_convert_array_load(J_INT, OPC_BALOAD, 0, 1);
	assert_convert_array_load(J_INT, OPC_BALOAD, 1, 2);
}

void test_convert_caload(void)
{
	assert_convert_array_load(J_CHAR, OPC_CALOAD, 0, 1);
	assert_convert_array_load(J_CHAR, OPC_CALOAD, 1, 2);
}

void test_convert_saload(void)
{
	assert_convert_array_load(J_SHORT, OPC_SALOAD, 0, 1);
	assert_convert_array_load(J_SHORT, OPC_SALOAD, 1, 2);
}

static void __assert_convert_store(unsigned char *code, unsigned long size,
				   enum jvm_type expected_jvm_type,
				   unsigned char expected_index,
				   unsigned long expected_temporary)
{
	struct statement *stmt;
	struct compilation_unit *cu;
	struct methodblock method = {
		.code = code,
		.code_size = size,
	};

	cu = alloc_simple_compilation_unit(&method);

	stack_push(cu->expr_stack, temporary_expr(J_INT, expected_temporary));

	convert_to_ir(cu);
	stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);

	assert_store_stmt(stmt);
	assert_temporary_expr(expected_temporary, stmt->store_src);
	assert_local_expr(expected_jvm_type, expected_index, stmt->store_dest);

	assert_true(stack_is_empty(cu->expr_stack));

	free_compilation_unit(cu);
}

static void assert_convert_store(unsigned char opc,
				 enum jvm_type expected_jvm_type,
				 unsigned char expected_index,
				 unsigned long expected_temporary)
{
	unsigned char code[] = { opc, expected_index };
	__assert_convert_store(code, ARRAY_SIZE(code), expected_jvm_type,
			       expected_index, expected_temporary);
}

void test_convert_istore(void)
{
	assert_convert_store(OPC_ISTORE, J_INT, 0x00, 0x01);
	assert_convert_store(OPC_ISTORE, J_INT, 0x01, 0x02);
}

void test_convert_lstore(void)
{
	assert_convert_store(OPC_LSTORE, J_LONG, 0x00, 0x01);
	assert_convert_store(OPC_LSTORE, J_LONG, 0x01, 0x02);
}

void test_convert_fstore(void)
{
	assert_convert_store(OPC_FSTORE, J_FLOAT, 0x00, 0x01);
	assert_convert_store(OPC_FSTORE, J_FLOAT, 0x01, 0x02);
}

void test_convert_dstore(void)
{
	assert_convert_store(OPC_DSTORE, J_DOUBLE, 0x00, 0x01);
	assert_convert_store(OPC_DSTORE, J_DOUBLE, 0x01, 0x02);
}

void test_convert_astore(void)
{
	assert_convert_store(OPC_ASTORE, J_REFERENCE, 0x00, 0x01);
	assert_convert_store(OPC_ASTORE, J_REFERENCE, 0x01, 0x02);
}

static void assert_convert_store_n(unsigned char opc,
				 enum jvm_type expected_jvm_type,
				 unsigned char expected_index,
				 unsigned long expected_temporary)
{
	unsigned char code[] = { opc };
	__assert_convert_store(code, ARRAY_SIZE(code), expected_jvm_type,
			       expected_index, expected_temporary);
}

void test_convert_istore_n(void)
{
	assert_convert_store_n(OPC_ISTORE_0, J_INT, 0x00, 0xFF);
	assert_convert_store_n(OPC_ISTORE_1, J_INT, 0x01, 0xFF);
	assert_convert_store_n(OPC_ISTORE_2, J_INT, 0x02, 0xFF);
	assert_convert_store_n(OPC_ISTORE_3, J_INT, 0x03, 0xFF);
}

void test_convert_lstore_n(void)
{
	assert_convert_store_n(OPC_LSTORE_0, J_LONG, 0x00, 0xFF);
	assert_convert_store_n(OPC_LSTORE_1, J_LONG, 0x01, 0xFF);
	assert_convert_store_n(OPC_LSTORE_2, J_LONG, 0x02, 0xFF);
	assert_convert_store_n(OPC_LSTORE_3, J_LONG, 0x03, 0xFF);
}

void test_convert_fstore_n(void)
{
	assert_convert_store_n(OPC_FSTORE_0, J_FLOAT, 0x00, 0xFF);
	assert_convert_store_n(OPC_FSTORE_1, J_FLOAT, 0x01, 0xFF);
	assert_convert_store_n(OPC_FSTORE_2, J_FLOAT, 0x02, 0xFF);
	assert_convert_store_n(OPC_FSTORE_3, J_FLOAT, 0x03, 0xFF);
}

void test_convert_dstore_n(void)
{
	assert_convert_store_n(OPC_DSTORE_0, J_DOUBLE, 0x00, 0xFF);
	assert_convert_store_n(OPC_DSTORE_1, J_DOUBLE, 0x01, 0xFF);
	assert_convert_store_n(OPC_DSTORE_2, J_DOUBLE, 0x02, 0xFF);
	assert_convert_store_n(OPC_DSTORE_3, J_DOUBLE, 0x03, 0xFF);
}

void test_convert_astore_n(void)
{
	assert_convert_store_n(OPC_ASTORE_0, J_REFERENCE, 0x00, 0xFF);
	assert_convert_store_n(OPC_ASTORE_1, J_REFERENCE, 0x01, 0xFF);
	assert_convert_store_n(OPC_ASTORE_2, J_REFERENCE, 0x02, 0xFF);
	assert_convert_store_n(OPC_ASTORE_3, J_REFERENCE, 0x03, 0xFF);
}

static void assert_convert_array_store(enum jvm_type expected_type,
				       unsigned char opc,
				       unsigned long arrayref,
				       unsigned long index, unsigned long value)
{
	unsigned char code[] = { opc };
	struct expression *arrayref_expr, *index_expr, *expr;
	struct statement *stmt;
	struct compilation_unit *cu;
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};

	cu = alloc_simple_compilation_unit(&method);

	arrayref_expr = value_expr(J_REFERENCE, arrayref);
	index_expr = value_expr(J_INT, index);
	expr = temporary_expr(expected_type, value);

	stack_push(cu->expr_stack, arrayref_expr);
	stack_push(cu->expr_stack, index_expr);
	stack_push(cu->expr_stack, expr);

	convert_to_ir(cu);
	stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);

	struct statement *nullcheck = stmt;
	struct statement *arraycheck = stmt_entry(nullcheck->stmt_list_node.next);
	struct statement *store_stmt = stmt_entry(arraycheck->stmt_list_node.next);

	assert_null_check_stmt(arrayref_expr, nullcheck);
	assert_arraycheck_stmt(expected_type, arrayref_expr, index_expr,
			       arraycheck);

	assert_store_stmt(store_stmt);
	assert_array_deref_expr(expected_type, arrayref_expr, index_expr,
				store_stmt->store_dest);
	assert_temporary_expr(value, store_stmt->store_src);

	assert_true(stack_is_empty(cu->expr_stack));

	free_compilation_unit(cu);
}

void test_convert_iastore(void)
{
	assert_convert_array_store(J_INT, OPC_IASTORE, 0, 1, 2);
	assert_convert_array_store(J_INT, OPC_IASTORE, 2, 3, 4);
}

void test_convert_lastore(void)
{
	assert_convert_array_store(J_LONG, OPC_LASTORE, 0, 1, 2);
	assert_convert_array_store(J_LONG, OPC_LASTORE, 2, 3, 4);
}

void test_convert_fastore(void)
{
	assert_convert_array_store(J_FLOAT, OPC_FASTORE, 0, 1, 2);
	assert_convert_array_store(J_FLOAT, OPC_FASTORE, 2, 3, 4);
}

void test_convert_dastore(void)
{
	assert_convert_array_store(J_DOUBLE, OPC_DASTORE, 0, 1, 2);
	assert_convert_array_store(J_DOUBLE, OPC_DASTORE, 2, 3, 4);
}

void test_convert_aastore(void)
{
	assert_convert_array_store(J_REFERENCE, OPC_AASTORE, 0, 1, 2);
	assert_convert_array_store(J_REFERENCE, OPC_AASTORE, 2, 3, 4);
}

void test_convert_bastore(void)
{
	assert_convert_array_store(J_INT, OPC_BASTORE, 0, 1, 2);
	assert_convert_array_store(J_INT, OPC_BASTORE, 2, 3, 4);
}

void test_convert_castore(void)
{
	assert_convert_array_store(J_CHAR, OPC_CASTORE, 0, 1, 2);
	assert_convert_array_store(J_CHAR, OPC_CASTORE, 2, 3, 4);
}

void test_convert_sastore(void)
{
	assert_convert_array_store(J_SHORT, OPC_SASTORE, 0, 1, 2);
	assert_convert_array_store(J_SHORT, OPC_SASTORE, 2, 3, 4);
}

static void assert_pop_stack(unsigned char opc)
{
	unsigned char code[] = { opc };
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct compilation_unit *cu;
	struct expression *expr;

	expr = value_expr(J_INT, 1);
	cu = alloc_simple_compilation_unit(&method);
	stack_push(cu->expr_stack, expr);
	convert_to_ir(cu);
	assert_true(stack_is_empty(cu->expr_stack));

	expr_put(expr);
	free_compilation_unit(cu);
}

void test_convert_pop(void)
{
	assert_pop_stack(OPC_POP);
	assert_pop_stack(OPC_POP2);
}

static void assert_dup_stack(unsigned char opc, void *expected)
{
	unsigned char code[] = { opc };
	struct compilation_unit *cu;
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};

	cu = alloc_simple_compilation_unit(&method);

	stack_push(cu->expr_stack, expected);

	convert_to_ir(cu);
	assert_ptr_equals(stack_pop(cu->expr_stack), expected);
	assert_ptr_equals(stack_pop(cu->expr_stack), expected);
	assert_true(stack_is_empty(cu->expr_stack));

	free_compilation_unit(cu);
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
	struct compilation_unit *cu;
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};

	cu = alloc_simple_compilation_unit(&method);

	stack_push(cu->expr_stack, expected2);
	stack_push(cu->expr_stack, expected1);

	convert_to_ir(cu);
	assert_ptr_equals(stack_pop(cu->expr_stack), expected1);
	assert_ptr_equals(stack_pop(cu->expr_stack), expected2);
	assert_ptr_equals(stack_pop(cu->expr_stack), expected1);
	assert_true(stack_is_empty(cu->expr_stack));

	free_compilation_unit(cu);
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
	struct compilation_unit *cu;
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};

	cu = alloc_simple_compilation_unit(&method);

	stack_push(cu->expr_stack, expected3);
	stack_push(cu->expr_stack, expected2);
	stack_push(cu->expr_stack, expected1);

	convert_to_ir(cu);
	assert_ptr_equals(stack_pop(cu->expr_stack), expected1);
	assert_ptr_equals(stack_pop(cu->expr_stack), expected2);
	assert_ptr_equals(stack_pop(cu->expr_stack), expected3);
	assert_ptr_equals(stack_pop(cu->expr_stack), expected1);
	assert_true(stack_is_empty(cu->expr_stack));

	free_compilation_unit(cu);
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
	struct compilation_unit *cu;
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};

	cu = alloc_simple_compilation_unit(&method);

	stack_push(cu->expr_stack, expected1);
	stack_push(cu->expr_stack, expected2);

	convert_to_ir(cu);
	assert_ptr_equals(stack_pop(cu->expr_stack), expected1);
	assert_ptr_equals(stack_pop(cu->expr_stack), expected2);
	assert_true(stack_is_empty(cu->expr_stack));

	free_compilation_unit(cu);
}

void test_convert_swap(void)
{
	assert_swap_stack(OPC_SWAP, (void *)1, (void *)2);
	assert_swap_stack(OPC_SWAP, (void *)2, (void *)3);
}

static void assert_conversion_expr_stack(unsigned char opc,
					 enum jvm_type from_type,
					 enum jvm_type to_type)
{
	unsigned char code[] = { opc };
	struct expression *expression, *conversion_expression;
	struct compilation_unit *cu;
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};

	cu = alloc_simple_compilation_unit(&method);

	expression = temporary_expr(from_type, 1);
	stack_push(cu->expr_stack, expression);

	convert_to_ir(cu);
	conversion_expression = stack_pop(cu->expr_stack);
	assert_conv_expr(to_type, expression, &conversion_expression->node);
	assert_true(stack_is_empty(cu->expr_stack));

	expr_put(conversion_expression);
	free_compilation_unit(cu);
}

void test_convert_int_widening(void)
{
	assert_conversion_expr_stack(OPC_I2L, J_INT, J_LONG);
	assert_conversion_expr_stack(OPC_I2F, J_INT, J_FLOAT);
	assert_conversion_expr_stack(OPC_I2D, J_INT, J_DOUBLE);
}

void test_convert_long_conversion(void)
{
	assert_conversion_expr_stack(OPC_L2I, J_LONG, J_INT);
	assert_conversion_expr_stack(OPC_L2F, J_LONG, J_FLOAT);
	assert_conversion_expr_stack(OPC_L2D, J_LONG, J_DOUBLE);
}

void test_convert_float_conversion(void)
{
	assert_conversion_expr_stack(OPC_F2I, J_FLOAT, J_INT);
	assert_conversion_expr_stack(OPC_F2L, J_FLOAT, J_LONG);
	assert_conversion_expr_stack(OPC_F2D, J_FLOAT, J_DOUBLE);
}

void test_convert_double_conversion(void)
{
	assert_conversion_expr_stack(OPC_D2I, J_DOUBLE, J_INT);
	assert_conversion_expr_stack(OPC_D2L, J_DOUBLE, J_LONG);
	assert_conversion_expr_stack(OPC_D2F, J_DOUBLE, J_FLOAT);
}

void test_convert_int_narrowing(void)
{
	assert_conversion_expr_stack(OPC_I2B, J_INT, J_BYTE);
	assert_conversion_expr_stack(OPC_I2C, J_INT, J_CHAR);
	assert_conversion_expr_stack(OPC_I2S, J_INT, J_SHORT);
}

static void assert_convert_cmp(unsigned char opc, enum binary_operator op,
			       enum jvm_type type)
{
	unsigned char code[] = { opc };
	struct expression *left, *right, *cmp_expression;
	struct compilation_unit *cu;
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};

	cu = alloc_simple_compilation_unit(&method);

	left = temporary_expr(type, 1);
	right = temporary_expr(type, 2);

	stack_push(cu->expr_stack, left);
	stack_push(cu->expr_stack, right);

	convert_to_ir(cu);
	cmp_expression = stack_pop(cu->expr_stack);
	assert_binop_expr(J_INT, op, left, right, &cmp_expression->node);
	assert_true(stack_is_empty(cu->expr_stack));

	expr_put(cmp_expression);
	free_compilation_unit(cu);
}

void test_convert_cmp(void)
{
	assert_convert_cmp(OPC_LCMP, OP_CMP, J_LONG);
	assert_convert_cmp(OPC_FCMPL, OP_CMPL, J_FLOAT);
	assert_convert_cmp(OPC_FCMPG, OP_CMPG, J_FLOAT);
	assert_convert_cmp(OPC_DCMPL, OP_CMPL, J_DOUBLE);
	assert_convert_cmp(OPC_DCMPG, OP_CMPG, J_DOUBLE);
}

/* MISSING: jsr */

/* MISSING: ret */

/* MISSING: tableswitch */

/* MISSING: lookupswitch */

static void assert_convert_return(enum jvm_type jvm_type, unsigned char opc)
{
	struct expression *return_value;
	struct compilation_unit *cu;
	unsigned char code[] = { opc };
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct statement *ret_stmt;

	cu = alloc_simple_compilation_unit(&method);

	return_value = temporary_expr(jvm_type, 0);
	stack_push(cu->expr_stack, return_value);

	convert_to_ir(cu);
	ret_stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);
	assert_true(stack_is_empty(cu->expr_stack));
	assert_return_stmt(return_value, ret_stmt);

	free_compilation_unit(cu);
}

void test_convert_non_void_return(void)
{
	assert_convert_return(J_INT, OPC_IRETURN);
	assert_convert_return(J_LONG, OPC_LRETURN);
	assert_convert_return(J_FLOAT, OPC_FRETURN);
	assert_convert_return(J_DOUBLE, OPC_DRETURN);
	assert_convert_return(J_REFERENCE, OPC_ARETURN);
}

void test_convert_void_return(void)
{
	struct compilation_unit *cu;
	unsigned char code[] = { OPC_RETURN };
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct statement *ret_stmt;

	cu = alloc_simple_compilation_unit(&method);

	convert_to_ir(cu);
	ret_stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);
	assert_true(stack_is_empty(cu->expr_stack));
	assert_void_return_stmt(ret_stmt);

	free_compilation_unit(cu);
}

/* MISSING: getfield */

/* MISSING: putfield */

/* MISSING: new */

/* MISSING: newarray */

/* MISSING: anewarray */

/* MISSING: arraylength */

/* MISSING: athrow */

/* MISSING: checkcast */

/* MISSING: instanceof */

/* MISSING: monitorenter */

/* MISSING: monitorexit */

/* MISSING: wide */

/* MISSING: multianewarray */

/* MISSING: ifnull */

/* MISSING: ifnonnull */

/* MISSING: goto_w */

/* MISSING: jsr_w */

void test_converts_complete_basic_block(void)
{
	struct compilation_unit *cu;
	unsigned char code[] = { OPC_ILOAD_0, OPC_ILOAD_1, OPC_IADD, OPC_IRETURN };
	struct methodblock method = {
		.code = code,
		.code_size = ARRAY_SIZE(code),
	};
 
	cu = alloc_simple_compilation_unit(&method);
	convert_to_ir(cu);

	assert_false(list_is_empty(&bb_entry(cu->bb_list.next)->stmt_list));
	assert_true(stack_is_empty(cu->expr_stack));
	
	free_compilation_unit(cu);
}
