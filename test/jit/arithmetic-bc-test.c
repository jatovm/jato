/*
 * Copyright (C) 2005-2006  Pekka Enberg
 */

#include <jit/statement.h>
#include <jit/jit-compiler.h>
#include <vm/list.h>
#include <vm/stack.h>
#include <vm/system.h>

#include <libharness.h>
#include <stdlib.h>

#include <bc-test-utils.h>

static void assert_unary_op_expr(enum jvm_type jvm_type,
				 enum unary_operator unary_operator,
				 struct expression *expression,
				 struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	assert_int_equals(EXPR_UNARY_OP, expr_type(expr));
	assert_int_equals(jvm_type, expr->jvm_type);
	assert_int_equals(unary_operator, expr_unary_op(expr));
	assert_ptr_equals(expression, to_expr(expr->unary_expression));
}

static void assert_convert_binop(enum jvm_type jvm_type,
				 enum binary_operator binary_operator,
				 unsigned char opc)
{
	unsigned char code[] = { opc };
	struct expression *left, *right, *expr;
	struct compilation_unit *cu;
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code)
	};

	cu = alloc_simple_compilation_unit(&method);

	left = temporary_expr(jvm_type, 1);
	right = temporary_expr(jvm_type, 2);

	stack_push(cu->expr_stack, left);
	stack_push(cu->expr_stack, right);

	convert_to_ir(cu);
	expr = stack_pop(cu->expr_stack);

	assert_binop_expr(jvm_type, binary_operator, left, right, &expr->node);
	assert_true(stack_is_empty(cu->expr_stack));

	expr_put(expr);
	free_compilation_unit(cu);
}

void test_convert_add(void)
{
	assert_convert_binop(J_INT, OP_ADD, OPC_IADD);
	assert_convert_binop(J_LONG, OP_ADD, OPC_LADD);
	assert_convert_binop(J_FLOAT, OP_ADD, OPC_FADD);
	assert_convert_binop(J_DOUBLE, OP_ADD, OPC_DADD);
}

void test_convert_sub(void)
{
	assert_convert_binop(J_INT, OP_SUB, OPC_ISUB);
	assert_convert_binop(J_LONG, OP_SUB, OPC_LSUB);
	assert_convert_binop(J_FLOAT, OP_SUB, OPC_FSUB);
	assert_convert_binop(J_DOUBLE, OP_SUB, OPC_DSUB);
}

void test_convert_mul(void)
{
	assert_convert_binop(J_INT, OP_MUL, OPC_IMUL);
	assert_convert_binop(J_LONG, OP_MUL, OPC_LMUL);
	assert_convert_binop(J_FLOAT, OP_MUL, OPC_FMUL);
	assert_convert_binop(J_DOUBLE, OP_MUL, OPC_DMUL);
}

void test_convert_div(void)
{
	assert_convert_binop(J_INT, OP_DIV, OPC_IDIV);
	assert_convert_binop(J_LONG, OP_DIV, OPC_LDIV);
	assert_convert_binop(J_FLOAT, OP_DIV, OPC_FDIV);
	assert_convert_binop(J_DOUBLE, OP_DIV, OPC_DDIV);
}

void test_convert_rem(void)
{
	assert_convert_binop(J_INT, OP_REM, OPC_IREM);
	assert_convert_binop(J_LONG, OP_REM, OPC_LREM);
	assert_convert_binop(J_FLOAT, OP_REM, OPC_FREM);
	assert_convert_binop(J_DOUBLE, OP_REM, OPC_DREM);
}

static void assert_convert_unop(enum jvm_type jvm_type,
				enum unary_operator unary_operator,
				unsigned char opc)
{
	unsigned char code[] = { opc };
	struct expression *expression, *unary_expression;
	struct compilation_unit *cu;
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code)
	};

	cu = alloc_simple_compilation_unit(&method);

	expression = temporary_expr(jvm_type, 1);
	stack_push(cu->expr_stack, expression);

	convert_to_ir(cu);
	unary_expression = stack_pop(cu->expr_stack);

	assert_unary_op_expr(jvm_type, unary_operator, expression,
			     &unary_expression->node);
	assert_true(stack_is_empty(cu->expr_stack));

	expr_put(unary_expression);
	free_compilation_unit(cu);
}

void test_convert_neg(void)
{
	assert_convert_unop(J_INT, OP_NEG, OPC_INEG);
	assert_convert_unop(J_LONG, OP_NEG, OPC_LNEG);
	assert_convert_unop(J_FLOAT, OP_NEG, OPC_FNEG);
	assert_convert_unop(J_DOUBLE, OP_NEG, OPC_DNEG);
}

void test_convert_shl(void)
{
	assert_convert_binop(J_INT, OP_SHL, OPC_ISHL);
	assert_convert_binop(J_LONG, OP_SHL, OPC_LSHL);
}

void test_convert_shr(void)
{
	assert_convert_binop(J_INT, OP_SHR, OPC_ISHR);
	assert_convert_binop(J_LONG, OP_SHR, OPC_LSHR);
}

void test_convert_ushr(void)
{
	assert_convert_binop(J_INT, OP_USHR, OPC_IUSHR);
	assert_convert_binop(J_LONG, OP_USHR, OPC_LUSHR);
}

void test_convert_and(void)
{
	assert_convert_binop(J_INT, OP_AND, OPC_IAND);
	assert_convert_binop(J_LONG, OP_AND, OPC_LAND);
}

void test_convert_or(void)
{
	assert_convert_binop(J_INT, OP_OR, OPC_IOR);
	assert_convert_binop(J_LONG, OP_OR, OPC_LOR);
}

void test_convert_xor(void)
{
	assert_convert_binop(J_INT, OP_XOR, OPC_IXOR);
	assert_convert_binop(J_LONG, OP_XOR, OPC_LXOR);
}

static void assert_iinc_stmt(unsigned char expected_index,
			     unsigned char expected_value)
{
	unsigned char code[] = { OPC_IINC, expected_index, expected_value };
	struct statement *store_stmt;
	struct tree_node *local_expression, *const_expression;
	struct compilation_unit *cu;
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code)
	};

	cu = alloc_simple_compilation_unit(&method);

	convert_to_ir(cu);
	store_stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);
	local_expression = store_stmt->store_dest;
	const_expression = to_expr(store_stmt->store_src)->binary_right;

	assert_binop_expr(J_INT, OP_ADD, to_expr(local_expression),
			  to_expr(const_expression),
			  store_stmt->store_src);
	assert_local_expr(J_INT, expected_index, local_expression);
	assert_value_expr(J_INT, expected_value, const_expression);

	free_compilation_unit(cu);
}

void test_convert_iinc(void)
{
	assert_iinc_stmt(0, 1);
	assert_iinc_stmt(1, 2);
}


