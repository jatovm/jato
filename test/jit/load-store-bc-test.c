/*
 * Copyright (C) 2005-2006  Pekka Enberg
 */

#include <vm/byteorder.h>
#include <vm/system.h>
#include <vm/types.h>
#include <vm/vm.h>
#include <jit/expression.h>
#include <jit/compiler.h>
#include <libharness.h>
#include <string.h>
#include <test/vm.h>

#include "bc-test-utils.h"

static void __assert_convert_const(enum vm_type expected_vm_type,
				   long long expected_value,
				   struct methodblock *method)
{
	struct expression *expr;
	struct basic_block *bb;

	bb = __alloc_simple_bb(method);
	convert_to_ir(bb->b_parent);

	expr = stack_pop(bb->mimic_stack);
	assert_value_expr(expected_vm_type, expected_value, &expr->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	__free_simple_bb(bb);
}

static void assert_convert_const(enum vm_type expected_vm_type,
				 long long expected_value, char actual)
{
	unsigned char code[] = { actual };
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};
	__assert_convert_const(expected_vm_type, expected_value, &method);
}

static void assert_convert_fconst(enum vm_type expected_vm_type,
				  double expected_value, char actual)
{
	unsigned char code[] = { actual };
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct expression *expr;
	struct basic_block *bb;

	bb = __alloc_simple_bb(&method);
	convert_to_ir(bb->b_parent);

	expr = stack_pop(bb->mimic_stack);
	assert_fvalue_expr(expected_vm_type, expected_value, &expr->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	__free_simple_bb(bb);
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
		.jit_code = code,
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
		.jit_code = code,
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

/*
 * Limits
 */

#define J_INT_MAX 2147483647
#define J_INT_MIN (-J_INT_MAX - 1)

#define J_LONG_MAX ((long long) 2<<63)
#define J_LONG_MIN (-J_LONG_MAX - 1)

static void const_set_s4(ConstantPoolEntry *cp_infos, unsigned long idx, s4 value)
{
	cp_infos[idx] = value;
}

static void const_set_s8(ConstantPoolEntry *cp_infos, unsigned long idx, s8 value)
{
	*(long long *)&(cp_infos[idx]) = value;
}

static void const_set_float(ConstantPoolEntry *cp_infos, unsigned long idx, float value)
{
	*(float *)&(cp_infos[idx]) = value;
}

static void const_set_double(ConstantPoolEntry *cp_infos, unsigned long idx, double value)
{
	*(double *)&(cp_infos[idx]) = value;
}

#define NR_CP_ENTRIES 512

static void assert_convert_ldc(enum vm_type expected_vm_type,
			       long long expected_value, u1 cp_type)
{
	ConstantPoolEntry cp_infos[NR_CP_ENTRIES];
	u1 cp_types[NR_CP_ENTRIES];
	unsigned char code[] = { OPC_LDC, 0xff };
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct expression *expr;
	struct basic_block *bb;

	const_set_s4(cp_infos, 0xff, expected_value);
	cp_types[0xff] = cp_type;

	bb = __alloc_simple_bb(&method);
	convert_ir_const(bb->b_parent, cp_infos, NR_CP_ENTRIES, cp_types);

	expr = stack_pop(bb->mimic_stack);
	assert_value_expr(expected_vm_type, expected_value, &expr->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	__free_simple_bb(bb);
}

void test_convert_ldc(void)
{
	assert_convert_ldc(J_INT, 0, CONSTANT_Integer);
	assert_convert_ldc(J_INT, 1, CONSTANT_Integer);
	assert_convert_ldc(J_INT, J_INT_MIN, CONSTANT_Integer);
	assert_convert_ldc(J_INT, J_INT_MAX, CONSTANT_Integer);
}

static void assert_convert_ldc_string(enum vm_type expected_vm_type,
				      long long expected_value)
{
	ConstantPoolEntry cp_infos[NR_CP_ENTRIES];
	u1 cp_types[NR_CP_ENTRIES];
	unsigned char code[] = { OPC_LDC, 0xff };
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct expression *expr;
	struct basic_block *bb;

	const_set_s4(cp_infos, 0xff, 0x00);
	cp_types[0xff] = CONSTANT_String;

	bb = __alloc_simple_bb(&method);
	convert_ir_const(bb->b_parent, cp_infos, NR_CP_ENTRIES, cp_types);

	expr = stack_pop(bb->mimic_stack);
	assert_value_expr(expected_vm_type, expected_value, &expr->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	__free_simple_bb(bb);
}

void test_convert_ldc_string(void)
{
	assert_convert_ldc_string(J_REFERENCE, RESOLVED_STRING_CONSTANT);
}

static void assert_convert_ldc_float(float expected_value)
{
	struct expression *expr;
	ConstantPoolEntry cp_infos[NR_CP_ENTRIES];
	u1 cp_types[NR_CP_ENTRIES];
	unsigned char code[] = { OPC_LDC, 0xff };
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct basic_block *bb;

	const_set_float(cp_infos, 0xff, expected_value);
	cp_types[0xff] = CONSTANT_Float;

	bb = __alloc_simple_bb(&method);
	convert_ir_const(bb->b_parent, cp_infos, NR_CP_ENTRIES, cp_types);

	expr = stack_pop(bb->mimic_stack);
	assert_fvalue_expr(J_FLOAT, expected_value, &expr->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	__free_simple_bb(bb);
}

void test_convert_ldc_float(void)
{
	assert_convert_ldc_float(0.01f);
	assert_convert_ldc_float( 1.0f);
	assert_convert_ldc_float(-1.0f);
}

static void assert_convert_ldc_w(enum vm_type expected_vm_type,
				long long expected_value, u1 cp_type,
				unsigned char opcode)
{
	struct expression *expr;
	ConstantPoolEntry cp_infos[NR_CP_ENTRIES];
	u1 cp_types[NR_CP_ENTRIES];
	unsigned char code[] = { opcode, 0x01, 0x00 };
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct basic_block *bb;

	if (opcode == OPC_LDC_W)
		const_set_s4(cp_infos, 0x100, (s4) expected_value);
	else
		const_set_s8(cp_infos, 0x100, (s8) expected_value);
	cp_types[0x100] = cp_type;

	bb = __alloc_simple_bb(&method);
	convert_ir_const(bb->b_parent, cp_infos, NR_CP_ENTRIES, cp_types);

	expr = stack_pop(bb->mimic_stack);
	assert_value_expr(expected_vm_type, expected_value, &expr->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	__free_simple_bb(bb);
}

static void assert_convert_ldc_w_string(enum vm_type expected_vm_type, long long expected_value)
{
	struct expression *expr;
	ConstantPoolEntry cp_infos[NR_CP_ENTRIES];
	u1 cp_types[NR_CP_ENTRIES];
	unsigned char code[] = { OPC_LDC_W, 0x01, 0x00 };
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct basic_block *bb;

	const_set_s4(cp_infos, 0x100, 0x00);
	cp_types[0x100] = CONSTANT_String;

	bb = __alloc_simple_bb(&method);
	convert_ir_const(bb->b_parent, cp_infos, NR_CP_ENTRIES, cp_types);

	expr = stack_pop(bb->mimic_stack);
	assert_value_expr(expected_vm_type, expected_value, &expr->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	__free_simple_bb(bb);
}

void test_convert_ldc_w_int(void)
{
	assert_convert_ldc_w(J_INT, 0, CONSTANT_Integer, OPC_LDC_W);
	assert_convert_ldc_w(J_INT, 1, CONSTANT_Integer, OPC_LDC_W);
	assert_convert_ldc_w(J_INT, J_INT_MIN, CONSTANT_Integer, OPC_LDC_W);
	assert_convert_ldc_w(J_INT, J_INT_MAX, CONSTANT_Integer, OPC_LDC_W);
}

void test_convert_ldc2_w_long(void)
{
	assert_convert_ldc_w(J_LONG, 0, CONSTANT_Long, OPC_LDC2_W);
	assert_convert_ldc_w(J_LONG, 1, CONSTANT_Long, OPC_LDC2_W);
	assert_convert_ldc_w(J_LONG, J_LONG_MIN, CONSTANT_Long, OPC_LDC2_W);
	assert_convert_ldc_w(J_LONG, J_LONG_MAX, CONSTANT_Long, OPC_LDC2_W);
}

void test_convert_ldc_w_string(void)
{
	assert_convert_ldc_w_string(J_REFERENCE, RESOLVED_STRING_CONSTANT);
}

static void assert_convert_ldc_w_float(enum vm_type expected_vm_type,
				  double expected_value,
				  u1 cp_type, unsigned long opcode)
{
	ConstantPoolEntry cp_infos[NR_CP_ENTRIES];
	u1 cp_types[NR_CP_ENTRIES];
	unsigned char code[] = { opcode, 0x01, 0x00 };
	struct expression *expr;
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct basic_block *bb;

	if (opcode == OPC_LDC_W)
		const_set_float(cp_infos, 0x100, (float) expected_value);
	else
		const_set_double(cp_infos, 0x100, expected_value);
	cp_types[0x100] = cp_type;

	bb = __alloc_simple_bb(&method);
	convert_ir_const(bb->b_parent, cp_infos, NR_CP_ENTRIES, cp_types);

	expr = stack_pop(bb->mimic_stack);
	assert_fvalue_expr(expected_vm_type, expected_value, &expr->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	__free_simple_bb(bb);
}

void test_convert_ldc_w_float(void)
{
	assert_convert_ldc_w_float(J_FLOAT, 0.01f, CONSTANT_Float, OPC_LDC_W);
	assert_convert_ldc_w_float(J_FLOAT,  1.0f, CONSTANT_Float, OPC_LDC_W);
	assert_convert_ldc_w_float(J_FLOAT, -1.0f, CONSTANT_Float, OPC_LDC_W);
}

void test_convert_ldc2_w_double(void)
{
	assert_convert_ldc_w_float(J_DOUBLE, 0.01f, CONSTANT_Double, OPC_LDC2_W);
	assert_convert_ldc_w_float(J_DOUBLE,  1.0f, CONSTANT_Double, OPC_LDC2_W);
	assert_convert_ldc_w_float(J_DOUBLE, -1.0f, CONSTANT_Double, OPC_LDC2_W);
}

static void __assert_convert_load(unsigned char *code,
				  unsigned long size,
				  enum vm_type expected_vm_type,
				  unsigned char expected_index)
{
	struct expression *expr;
	struct methodblock method = {
		.jit_code = code,
		.code_size = size,
	};
	struct basic_block *bb;

	bb = __alloc_simple_bb(&method);

	convert_to_ir(bb->b_parent);

	expr = stack_pop(bb->mimic_stack);
	assert_local_expr(expected_vm_type, expected_index, &expr->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	__free_simple_bb(bb);
}

static void assert_convert_load(unsigned char opc,
				enum vm_type expected_vm_type,
				unsigned char expected_index)
{
	unsigned char code[] = { opc, expected_index };
	__assert_convert_load(code, ARRAY_SIZE(code), expected_vm_type,
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
				  enum vm_type expected_vm_type,
				  unsigned char expected_index)
{
	unsigned char code[] = { opc };
	__assert_convert_load(code, ARRAY_SIZE(code), expected_vm_type,
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

static void __assert_convert_store(unsigned char *code, unsigned long size,
				   enum vm_type expected_vm_type,
				   unsigned char expected_index)
{
	struct statement *stmt;
	struct methodblock method = {
		.jit_code = code,
		.code_size = size,
	};
	struct var_info *temporary;
	struct basic_block *bb;

	bb = __alloc_simple_bb(&method);

	temporary = get_var(bb->b_parent);
	stack_push(bb->mimic_stack, temporary_expr(J_INT, NULL, temporary));

	convert_to_ir(bb->b_parent);
	stmt = stmt_entry(bb->stmt_list.next);

	assert_store_stmt(stmt);
	assert_temporary_expr(stmt->store_src);
	assert_local_expr(expected_vm_type, expected_index, stmt->store_dest);

	assert_true(stack_is_empty(bb->mimic_stack));

	__free_simple_bb(bb);
}

static void assert_convert_store(unsigned char opc,
				 enum vm_type expected_vm_type,
				 unsigned char expected_index)
{
	unsigned char code[] = { opc, expected_index };
	__assert_convert_store(code, ARRAY_SIZE(code), expected_vm_type,
			       expected_index);
}

void test_convert_istore(void)
{
	assert_convert_store(OPC_ISTORE, J_INT, 0x00);
	assert_convert_store(OPC_ISTORE, J_INT, 0x01);
}

void test_convert_lstore(void)
{
	assert_convert_store(OPC_LSTORE, J_LONG, 0x00);
	assert_convert_store(OPC_LSTORE, J_LONG, 0x01);
}

void test_convert_fstore(void)
{
	assert_convert_store(OPC_FSTORE, J_FLOAT, 0x00);
	assert_convert_store(OPC_FSTORE, J_FLOAT, 0x01);
}

void test_convert_dstore(void)
{
	assert_convert_store(OPC_DSTORE, J_DOUBLE, 0x00);
	assert_convert_store(OPC_DSTORE, J_DOUBLE, 0x01);
}

void test_convert_astore(void)
{
	assert_convert_store(OPC_ASTORE, J_REFERENCE, 0x00);
	assert_convert_store(OPC_ASTORE, J_REFERENCE, 0x01);
}

static void assert_convert_store_n(unsigned char opc,
				 enum vm_type expected_vm_type,
				 unsigned char expected_index)
{
	unsigned char code[] = { opc };
	__assert_convert_store(code, ARRAY_SIZE(code), expected_vm_type,
			       expected_index);
}

void test_convert_istore_n(void)
{
	assert_convert_store_n(OPC_ISTORE_0, J_INT, 0x00);
	assert_convert_store_n(OPC_ISTORE_1, J_INT, 0x01);
	assert_convert_store_n(OPC_ISTORE_2, J_INT, 0x02);
	assert_convert_store_n(OPC_ISTORE_3, J_INT, 0x03);
}

void test_convert_lstore_n(void)
{
	assert_convert_store_n(OPC_LSTORE_0, J_LONG, 0x00);
	assert_convert_store_n(OPC_LSTORE_1, J_LONG, 0x01);
	assert_convert_store_n(OPC_LSTORE_2, J_LONG, 0x02);
	assert_convert_store_n(OPC_LSTORE_3, J_LONG, 0x03);
}

void test_convert_fstore_n(void)
{
	assert_convert_store_n(OPC_FSTORE_0, J_FLOAT, 0x00);
	assert_convert_store_n(OPC_FSTORE_1, J_FLOAT, 0x01);
	assert_convert_store_n(OPC_FSTORE_2, J_FLOAT, 0x02);
	assert_convert_store_n(OPC_FSTORE_3, J_FLOAT, 0x03);
}

void test_convert_dstore_n(void)
{
	assert_convert_store_n(OPC_DSTORE_0, J_DOUBLE, 0x00);
	assert_convert_store_n(OPC_DSTORE_1, J_DOUBLE, 0x01);
	assert_convert_store_n(OPC_DSTORE_2, J_DOUBLE, 0x02);
	assert_convert_store_n(OPC_DSTORE_3, J_DOUBLE, 0x03);
}

void test_convert_astore_n(void)
{
	assert_convert_store_n(OPC_ASTORE_0, J_REFERENCE, 0x00);
	assert_convert_store_n(OPC_ASTORE_1, J_REFERENCE, 0x01);
	assert_convert_store_n(OPC_ASTORE_2, J_REFERENCE, 0x02);
	assert_convert_store_n(OPC_ASTORE_3, J_REFERENCE, 0x03);
}

