/*
 * Copyright (C) 2005-2006  Pekka Enberg
 */

#include <cafebabe/constant_pool.h>

#include <vm/system.h>
#include <vm/types.h>
#include <vm/vm.h>
#include <vm/limits.h>
#include <jit/expression.h>
#include <jit/compiler.h>
#include <libharness.h>
#include <string.h>
#include <test/vm.h>

#include "bc-test-utils.h"

static void __assert_convert_const(enum vm_type expected_type,
				   long long expected_value,
				   unsigned char *code,
				   unsigned long code_size)
{
	struct expression *expr;
	struct basic_block *bb;

	bb = alloc_simple_bb(code, code_size);
	convert_to_ir(bb->b_parent);

	expr = stack_pop(bb->mimic_stack);
	assert_value_expr(expected_type, expected_value, &expr->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	free_simple_bb(bb);
}

static void assert_convert_const(enum vm_type expected_type,
				 long long expected_value,
				 unsigned char opc)
{
	__assert_convert_const(expected_type, expected_value, &opc, 1);
}

static void assert_convert_bipush(long long expected_value,
				  unsigned char byte, char opc)
{
	unsigned char code[] = { opc, byte };

	__assert_convert_const(J_INT, expected_value, code, ARRAY_SIZE(code));
}

static void assert_convert_sipush(long long expected_value,
				  unsigned char high_byte,
				  unsigned char low_byte,
				  unsigned char opc)
{
	unsigned char code[] = { opc, high_byte, low_byte };

	__assert_convert_const(J_INT, expected_value, code, ARRAY_SIZE(code));
}

static void assert_convert_fconst(enum vm_type expected_type,
				  double expected_value, unsigned char opc)
{
	struct expression *expr;
	struct basic_block *bb;

	bb = alloc_simple_bb(&opc, 1);
	convert_to_ir(bb->b_parent);

	expr = stack_pop(bb->mimic_stack);
	assert_fvalue_expr(expected_type, expected_value, &expr->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	free_simple_bb(bb);
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

void test_convert_bipush(void)
{
	assert_convert_bipush(0, 0x00, OPC_BIPUSH);
	assert_convert_bipush(1, 0x01, OPC_BIPUSH);
	assert_convert_bipush(J_BYTE_MIN, 0x80, OPC_BIPUSH);
	assert_convert_bipush(J_BYTE_MAX, 0x7F, OPC_BIPUSH);
}

void test_convert_sipush(void)
{
	assert_convert_sipush(0, 0x00, 0x00, OPC_SIPUSH);
	assert_convert_sipush(1, 0x00, 0x01, OPC_SIPUSH);
	assert_convert_sipush(J_SHORT_MIN, 0x80, 0x00, OPC_SIPUSH);
	assert_convert_sipush(J_SHORT_MAX, 0x7F, 0xFF, OPC_SIPUSH);
}

static void const_set_int32_t(ConstantPoolEntry *cp_infos, unsigned long idx, int32_t value)
{
	cp_infos[idx] = value;
}

static void const_set_int64_t(ConstantPoolEntry *cp_infos, unsigned long idx, int64_t value)
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

static void assert_convert_ldc(enum vm_type expected_type,
			       long long expected_value, uint8_t cp_type)
{
	ConstantPoolEntry cp_infos[NR_CP_ENTRIES];
	uint8_t cp_types[NR_CP_ENTRIES];
	unsigned char code[] = { OPC_LDC, 0xff };
	struct expression *expr;
	struct basic_block *bb;

	const_set_int32_t(cp_infos, 0xff, expected_value);
	cp_types[0xff] = cp_type;

	bb = alloc_simple_bb(code, ARRAY_SIZE(code));
	convert_ir_const(bb->b_parent, cp_infos, NR_CP_ENTRIES, cp_types);

	expr = stack_pop(bb->mimic_stack);
	assert_value_expr(expected_type, expected_value, &expr->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	free_simple_bb(bb);
}

void test_convert_ldc(void)
{
	NOT_IMPLEMENTED;

#if 0
	assert_convert_ldc(J_INT, 0, CONSTANT_Integer);
	assert_convert_ldc(J_INT, 1, CONSTANT_Integer);
	assert_convert_ldc(J_INT, J_INT_MIN, CONSTANT_Integer);
	assert_convert_ldc(J_INT, J_INT_MAX, CONSTANT_Integer);
#endif
}

static void assert_convert_ldc_string(enum vm_type expected_type,
				      long long expected_value)
{
	unsigned char code[] = { OPC_LDC, 0xff };
	ConstantPoolEntry cp_infos[NR_CP_ENTRIES];
	uint8_t cp_types[NR_CP_ENTRIES];
	struct expression *expr;
	struct basic_block *bb;

	const_set_int32_t(cp_infos, 0xff, 0x00);
	cp_types[0xff] = CAFEBABE_CONSTANT_TAG_STRING;

	bb = alloc_simple_bb(code, ARRAY_SIZE(code));
	convert_ir_const(bb->b_parent, cp_infos, NR_CP_ENTRIES, cp_types);

	expr = stack_pop(bb->mimic_stack);
	assert_value_expr(expected_type, expected_value, &expr->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	free_simple_bb(bb);
}

void test_convert_ldc_string(void)
{
	NOT_IMPLEMENTED;

#if 0
	assert_convert_ldc_string(J_REFERENCE, RESOLVED_STRING_CONSTANT);
#endif
}

static void assert_convert_ldc_float(float expected_value)
{
	unsigned char code[] = { OPC_LDC, 0xff };
	ConstantPoolEntry cp_infos[NR_CP_ENTRIES];
	uint8_t cp_types[NR_CP_ENTRIES];
	struct expression *expr;
	struct basic_block *bb;

	const_set_float(cp_infos, 0xff, expected_value);
	cp_types[0xff] = CAFEBABE_CONSTANT_TAG_FLOAT;

	bb = alloc_simple_bb(code, ARRAY_SIZE(code));
	convert_ir_const(bb->b_parent, cp_infos, NR_CP_ENTRIES, cp_types);

	expr = stack_pop(bb->mimic_stack);
	assert_fvalue_expr(J_FLOAT, expected_value, &expr->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	free_simple_bb(bb);
}

void test_convert_ldc_float(void)
{
	NOT_IMPLEMENTED;

#if 0
	assert_convert_ldc_float(0.01f);
	assert_convert_ldc_float( 1.0f);
	assert_convert_ldc_float(-1.0f);
#endif
}

static void assert_convert_ldc_w(enum vm_type expected_type,
				long long expected_value, uint8_t cp_type,
				unsigned char opcode)
{
	unsigned char code[] = { opcode, 0x01, 0x00 };
	ConstantPoolEntry cp_infos[NR_CP_ENTRIES];
	uint8_t cp_types[NR_CP_ENTRIES];
	struct basic_block *bb;
	struct expression *expr;

	if (opcode == OPC_LDC_W)
		const_set_int32_t(cp_infos, 0x100, (int32_t) expected_value);
	else
		const_set_int64_t(cp_infos, 0x100, (int64_t) expected_value);
	cp_types[0x100] = cp_type;

	bb = alloc_simple_bb(code, ARRAY_SIZE(code));
	convert_ir_const(bb->b_parent, cp_infos, NR_CP_ENTRIES, cp_types);

	expr = stack_pop(bb->mimic_stack);
	assert_value_expr(expected_type, expected_value, &expr->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	free_simple_bb(bb);
}

static void assert_convert_ldc_w_string(enum vm_type expected_type, long long expected_value)
{
	unsigned char code[] = { OPC_LDC_W, 0x01, 0x00 };
	ConstantPoolEntry cp_infos[NR_CP_ENTRIES];
	uint8_t cp_types[NR_CP_ENTRIES];
	struct basic_block *bb;
	struct expression *expr;

	const_set_int32_t(cp_infos, 0x100, 0x00);
	cp_types[0x100] = CAFEBABE_CONSTANT_TAG_STRING;

	bb = alloc_simple_bb(code, ARRAY_SIZE(code));
	convert_ir_const(bb->b_parent, cp_infos, NR_CP_ENTRIES, cp_types);

	expr = stack_pop(bb->mimic_stack);
	assert_value_expr(expected_type, expected_value, &expr->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	free_simple_bb(bb);
}

void test_convert_ldc_w_int(void)
{
	NOT_IMPLEMENTED;

#if 0
	assert_convert_ldc_w(J_INT, 0, CONSTANT_Integer, OPC_LDC_W);
	assert_convert_ldc_w(J_INT, 1, CONSTANT_Integer, OPC_LDC_W);
	assert_convert_ldc_w(J_INT, J_INT_MIN, CONSTANT_Integer, OPC_LDC_W);
	assert_convert_ldc_w(J_INT, J_INT_MAX, CONSTANT_Integer, OPC_LDC_W);
#endif
}

void test_convert_ldc2_w_long(void)
{
	NOT_IMPLEMENTED;

#if 0
	assert_convert_ldc_w(J_LONG, 0, CONSTANT_Long, OPC_LDC2_W);
	assert_convert_ldc_w(J_LONG, 1, CONSTANT_Long, OPC_LDC2_W);
	assert_convert_ldc_w(J_LONG, J_LONG_MIN, CONSTANT_Long, OPC_LDC2_W);
	assert_convert_ldc_w(J_LONG, J_LONG_MAX, CONSTANT_Long, OPC_LDC2_W);
#endif
}

void test_convert_ldc_w_string(void)
{
	NOT_IMPLEMENTED;

#if 0
	assert_convert_ldc_w_string(J_REFERENCE, RESOLVED_STRING_CONSTANT);
#endif
}

static void assert_convert_ldc_w_float(enum vm_type expected_type,
				  double expected_value,
				  uint8_t cp_type, unsigned long opcode)
{
	unsigned char code[] = { opcode, 0x01, 0x00 };
	ConstantPoolEntry cp_infos[NR_CP_ENTRIES];
	uint8_t cp_types[NR_CP_ENTRIES];
	struct expression *expr;
	struct basic_block *bb;

	if (opcode == OPC_LDC_W)
		const_set_float(cp_infos, 0x100, (float) expected_value);
	else
		const_set_double(cp_infos, 0x100, expected_value);
	cp_types[0x100] = cp_type;

	bb = alloc_simple_bb(code, ARRAY_SIZE(code));
	convert_ir_const(bb->b_parent, cp_infos, NR_CP_ENTRIES, cp_types);

	expr = stack_pop(bb->mimic_stack);
	assert_fvalue_expr(expected_type, expected_value, &expr->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	free_simple_bb(bb);
}

void test_convert_ldc_w_float(void)
{
	NOT_IMPLEMENTED;

#if 0
	assert_convert_ldc_w_float(J_FLOAT, 0.01f, CAFEBABE_CONSTANT_TAG_FLOAT, OPC_LDC_W);
	assert_convert_ldc_w_float(J_FLOAT,  1.0f, CAFEBABE_CONSTANT_TAG_FLOAT, OPC_LDC_W);
	assert_convert_ldc_w_float(J_FLOAT, -1.0f, CAFEBABE_CONSTANT_TAG_FLOAT, OPC_LDC_W);
#endif
}

void test_convert_ldc2_w_double(void)
{
	NOT_IMPLEMENTED;

#if 0
	assert_convert_ldc_w_float(J_DOUBLE, 0.01f, CAFEBABE_CONSTANT_TAG_DOUBLE, OPC_LDC2_W);
	assert_convert_ldc_w_float(J_DOUBLE,  1.0f, CAFEBABE_CONSTANT_TAG_DOUBLE, OPC_LDC2_W);
	assert_convert_ldc_w_float(J_DOUBLE, -1.0f, CAFEBABE_CONSTANT_TAG_DOUBLE, OPC_LDC2_W);
#endif
}

static void __assert_convert_load(unsigned char *code,
				  unsigned long code_size,
				  enum vm_type expected_type,
				  unsigned char expected_index)
{
	struct expression *expr;
	struct statement *stmt;
	struct basic_block *bb;

	bb = alloc_simple_bb(code, code_size);

	convert_to_ir(bb->b_parent);

	expr = stack_pop(bb->mimic_stack);
	assert_temporary_expr(&expr->node);

	stmt = stmt_entry(bb->stmt_list.next);

	assert_store_stmt(stmt);
	assert_local_expr(expected_type, expected_index, stmt->store_src);
	assert_ptr_equals(&expr->node, stmt->store_dest);

	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	free_simple_bb(bb);
}

static void assert_convert_load(unsigned char opc,
				enum vm_type expected_type,
				unsigned char expected_index)
{
	unsigned char code[] = { opc, expected_index };

	__assert_convert_load(code, ARRAY_SIZE(code), expected_type, expected_index);
}

static void assert_convert_load_n(unsigned char opc,
				  enum vm_type expected_type,
				  unsigned char expected_index)
{
	__assert_convert_load(&opc, 1, expected_type, expected_index);
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

static void __assert_convert_store(unsigned char *code,
				   unsigned long code_size,
				   enum vm_type expected_type,
				   unsigned char expected_index)
{
	struct statement *stmt;
	struct var_info *temporary;
	struct basic_block *bb;

	bb = alloc_simple_bb(code, code_size);

	temporary = get_var(bb->b_parent);
	stack_push(bb->mimic_stack, temporary_expr(J_INT, NULL, temporary));

	convert_to_ir(bb->b_parent);
	stmt = stmt_entry(bb->stmt_list.next);

	assert_store_stmt(stmt);
	assert_temporary_expr(stmt->store_src);
	assert_local_expr(expected_type, expected_index, stmt->store_dest);

	assert_true(stack_is_empty(bb->mimic_stack));

	free_simple_bb(bb);
}

static void assert_convert_store(unsigned char opc,
				 enum vm_type expected_type,
				 unsigned char expected_index)
{
	unsigned char code[] = { opc, expected_index };

	__assert_convert_store(code, ARRAY_SIZE(code), expected_type,
			       expected_index);
}

static void assert_convert_store_n(unsigned char opc,
				 enum vm_type expected_type,
				 unsigned char expected_index)
{
	__assert_convert_store(&opc, 1, expected_type, expected_index);
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

