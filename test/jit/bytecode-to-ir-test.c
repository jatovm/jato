/*
 * Copyright (C) 2005  Pekka Enberg
 */

#include <jit/jit-compiler.h>
#include <jit/bytecode-converters.h>
#include <jit/statement.h>
#include <vm/list.h>
#include <vm/stack.h>
#include <vm/system.h>

#include <bc-test-utils.h>
#include <libharness.h>
#include <stdlib.h>

void test_convert_nop(void)
{
	unsigned char code[] = { OPC_NOP };
	struct methodblock method = {
		.jit_code = code,
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

static void assert_convert_array_load(enum jvm_type expected_type,
				      unsigned char opc,
				      unsigned long arrayref,
				      unsigned long index)
{
	unsigned char code[] = { opc };
	struct expression *arrayref_expr, *index_expr, *temporary_expr;
	struct statement *stmt;
	struct methodblock method = {
		.jit_code = code,
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
		.jit_code = code,
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
		.jit_code = code,
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
		.jit_code = code,
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
		.jit_code = code,
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
		.jit_code = code,
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
		.jit_code = code,
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
		.jit_code = code,
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
		.jit_code = code,
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
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};
 
	cu = alloc_simple_compilation_unit(&method);
	convert_to_ir(cu);

	assert_false(list_is_empty(&bb_entry(cu->bb_list.next)->stmt_list));
	assert_true(stack_is_empty(cu->expr_stack));
	
	free_compilation_unit(cu);
}
