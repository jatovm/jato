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
