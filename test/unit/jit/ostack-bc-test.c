/*
 * Copyright (C) 2005-2006  Pekka Enberg
 */

#include "vm/vm.h"
#include "jit/expression.h"
#include "jit/compiler.h"
#include <stdlib.h>
#include <libharness.h>

#include "bc-test-utils.h"

/* The returned pointer could be stale because of expr_put() so only use the
   return value for pointer comparison.  */
static struct expression *pop_and_put_expr(struct stack *stack)
{
	struct expression *expr;

	expr = stack_pop(stack);
	expr_put(expr);

	return expr;
}

static void assert_dup_stack(unsigned char opc, struct expression *value)
{
	struct basic_block *bb;
	struct statement *stmt;

	bb = alloc_simple_bb(&opc, 1);
	stack_push(bb->mimic_stack, expr_get(value));

	convert_to_ir(bb->b_parent);
        stmt = stmt_entry(bb->stmt_list.next);

	assert_store_stmt(stmt);
	assert_ptr_equals(value, to_expr(stmt->store_src));
	assert_temporary_expr(value->vm_type, stmt->store_dest);

	assert_ptr_equals(to_expr(stmt->store_dest), pop_and_put_expr(bb->mimic_stack));
	assert_ptr_equals(value, pop_and_put_expr(bb->mimic_stack));

	assert_true(stack_is_empty(bb->mimic_stack));

	free_simple_bb(bb);
}

static void assert_dup2_stack(unsigned char opc, struct expression *value, struct expression *value2)
{
	struct statement *stmt, *stmt2;
	struct basic_block *bb;

	if (value->vm_type == J_LONG || value->vm_type == J_DOUBLE) {
		assert_dup_stack(opc, value);
		return;
	}

	bb = alloc_simple_bb(&opc, 1);

	stack_push(bb->mimic_stack, expr_get(value2));
	stack_push(bb->mimic_stack, expr_get(value));

	convert_to_ir(bb->b_parent);
	stmt = stmt_entry(bb->stmt_list.next);
	stmt2 = stmt_entry(stmt->stmt_list_node.next);

	assert_store_stmt(stmt);
	assert_ptr_equals(value2, to_expr(stmt->store_src));
	assert_temporary_expr(value2->vm_type, stmt->store_dest);

	assert_store_stmt(stmt2);
	assert_ptr_equals(value, to_expr(stmt2->store_src));
	assert_temporary_expr(value->vm_type, stmt->store_dest);

	assert_ptr_equals(to_expr(stmt2->store_dest), pop_and_put_expr(bb->mimic_stack));
	assert_ptr_equals(to_expr(stmt->store_dest), pop_and_put_expr(bb->mimic_stack));
	assert_ptr_equals(value, pop_and_put_expr(bb->mimic_stack));
	assert_ptr_equals(value2, pop_and_put_expr(bb->mimic_stack));

	assert_true(stack_is_empty(bb->mimic_stack));

	free_simple_bb(bb);
}

void test_convert_dup(void)
{
	struct expression *value1, *value2, *value3;

	value1 = value_expr(J_REFERENCE, 0xdeadbeef);
	value2 = value_expr(J_REFERENCE, 0xcafedeca);
	value3 = value_expr(J_LONG, 0xcafecafecafecafe);

	assert_dup_stack(OPC_DUP, value1);
	assert_dup2_stack(OPC_DUP2, value1, value2);
	assert_dup2_stack(OPC_DUP2, value3, NULL);

	expr_put(value1);
	expr_put(value2);
	expr_put(value3);
}

static void assert_dup_x1_stack(unsigned char opc, struct expression *value1,
				struct expression *value2)
{
	struct basic_block *bb;
	struct statement *stmt;

	bb = alloc_simple_bb(&opc, 1);

	stack_push(bb->mimic_stack, expr_get(value2));
	stack_push(bb->mimic_stack, expr_get(value1));

	convert_to_ir(bb->b_parent);
        stmt = stmt_entry(bb->stmt_list.next);

	assert_store_stmt(stmt);
	assert_ptr_equals(value1, to_expr(stmt->store_src));
	assert_temporary_expr(value1->vm_type, stmt->store_dest);

	assert_ptr_equals(to_expr(stmt->store_dest), pop_and_put_expr(bb->mimic_stack));
	assert_ptr_equals(value2, pop_and_put_expr(bb->mimic_stack));
	assert_ptr_equals(value1, pop_and_put_expr(bb->mimic_stack));

	assert_true(stack_is_empty(bb->mimic_stack));

	free_simple_bb(bb);
}

static void assert_dup2_x1_stack(unsigned char opc, struct expression *value1,
				struct expression *value2, struct expression *value3)
{
	struct statement *stmt, *stmt2;
	struct basic_block *bb;

	if (value1->vm_type == J_LONG || value2->vm_type == J_DOUBLE) {
		assert_dup_x1_stack(opc, value1, value2);
		return;
	}

	bb = alloc_simple_bb(&opc, 1);

	stack_push(bb->mimic_stack, expr_get(value3));
	stack_push(bb->mimic_stack, expr_get(value2));
	stack_push(bb->mimic_stack, expr_get(value1));

	convert_to_ir(bb->b_parent);
        stmt = stmt_entry(bb->stmt_list.next);
	stmt2 = stmt_entry(stmt->stmt_list_node.next);

	assert_store_stmt(stmt);
	assert_ptr_equals(value2, to_expr(stmt->store_src));
	assert_temporary_expr(value2->vm_type, stmt->store_dest);

	assert_store_stmt(stmt2);
	assert_ptr_equals(value1, to_expr(stmt2->store_src));
	assert_temporary_expr(value1->vm_type, stmt2->store_dest);

	assert_ptr_equals(to_expr(stmt2->store_dest), pop_and_put_expr(bb->mimic_stack));
	assert_ptr_equals(to_expr(stmt->store_dest), pop_and_put_expr(bb->mimic_stack));
	assert_ptr_equals(value3, pop_and_put_expr(bb->mimic_stack));
	assert_ptr_equals(value1, pop_and_put_expr(bb->mimic_stack));
	assert_ptr_equals(value2, pop_and_put_expr(bb->mimic_stack));

	assert_true(stack_is_empty(bb->mimic_stack));

	free_simple_bb(bb);
}

void test_convert_dup_x1(void)
{
	struct expression *value1, *value2, *value3;

	value1 = value_expr(J_REFERENCE, 0xdeadbeef);
	value2 = value_expr(J_REFERENCE, 0xcafebabe);
	value3 = value_expr(J_LONG, 0xdecacafebabebeef);

	assert_dup_x1_stack(OPC_DUP_X1, value1, value2);
	assert_dup2_x1_stack(OPC_DUP2_X1, value1, value2, value3);
	assert_dup2_x1_stack(OPC_DUP2_X1, value3, value2, value1);

	expr_put(value1);
	expr_put(value2);
	expr_put(value3);
}

static void assert_dup_x2_stack(unsigned char opc, struct expression *value1,
				struct expression *value2, struct expression *value3)
{
	struct basic_block *bb;
	struct statement *stmt;

	bb = alloc_simple_bb(&opc, 1);

	stack_push(bb->mimic_stack, expr_get(value3));
	stack_push(bb->mimic_stack, expr_get(value2));
	stack_push(bb->mimic_stack, expr_get(value1));

	convert_to_ir(bb->b_parent);
        stmt = stmt_entry(bb->stmt_list.next);

	assert_store_stmt(stmt);
	assert_ptr_equals(value1, to_expr(stmt->store_src));
	assert_temporary_expr(value1->vm_type, stmt->store_dest);

	assert_ptr_equals(to_expr(stmt->store_dest), pop_and_put_expr(bb->mimic_stack));
	assert_ptr_equals(value2, pop_and_put_expr(bb->mimic_stack));
	assert_ptr_equals(value3, pop_and_put_expr(bb->mimic_stack));
	assert_ptr_equals(value1, pop_and_put_expr(bb->mimic_stack));

	assert_true(stack_is_empty(bb->mimic_stack));

	free_simple_bb(bb);
}

static void assert_dup2_x2_stack(unsigned char opc, struct expression *value1,
				struct expression *value2, struct expression *value3,
				struct expression *value4)
{
	struct statement *stmt, *stmt2;
	struct basic_block *bb;

	if (value1->vm_type == J_LONG || value1->vm_type == J_DOUBLE) {
		if (value2->vm_type == J_LONG || value2->vm_type == J_DOUBLE) {
			assert_dup_x1_stack(opc, value1, value2);
			return;
		} else {
			assert_dup_x2_stack(opc, value1, value2, value3);
			return;
		}
	} else {
		if (value3->vm_type == J_LONG || value3->vm_type == J_DOUBLE) {
			assert_dup2_x1_stack(opc, value1, value2, value3);
			return;
		}
	}

	bb = alloc_simple_bb(&opc, 1);

	stack_push(bb->mimic_stack, expr_get(value4));
	stack_push(bb->mimic_stack, expr_get(value3));
	stack_push(bb->mimic_stack, expr_get(value2));
	stack_push(bb->mimic_stack, expr_get(value1));

	convert_to_ir(bb->b_parent);
        stmt = stmt_entry(bb->stmt_list.next);
	stmt2 = stmt_entry(stmt->stmt_list_node.next);

	assert_store_stmt(stmt);
	assert_ptr_equals(value2, to_expr(stmt->store_src));
	assert_temporary_expr(value2->vm_type, stmt->store_dest);

	assert_store_stmt(stmt2);
	assert_ptr_equals(value1, to_expr(stmt2->store_src));
	assert_temporary_expr(value1->vm_type, stmt2->store_dest);

	assert_ptr_equals(to_expr(stmt2->store_dest), pop_and_put_expr(bb->mimic_stack));
	assert_ptr_equals(to_expr(stmt->store_dest), pop_and_put_expr(bb->mimic_stack));
	assert_ptr_equals(value3, pop_and_put_expr(bb->mimic_stack));
	assert_ptr_equals(value4, pop_and_put_expr(bb->mimic_stack));
	assert_ptr_equals(value1, pop_and_put_expr(bb->mimic_stack));
	assert_ptr_equals(value2, pop_and_put_expr(bb->mimic_stack));

	assert_true(stack_is_empty(bb->mimic_stack));

	free_simple_bb(bb);
}

void test_convert_dup_x2(void)
{
	struct expression *value1, *value2, *value3, *value4;

	value1 = value_expr(J_REFERENCE, 0xdeadbeef);
	value2 = value_expr(J_REFERENCE, 0xcafebabe);
	value3 = value_expr(J_REFERENCE, 0xb4df00d);
	value4 = value_expr(J_REFERENCE, 0x6559570);

	assert_dup_x2_stack(OPC_DUP_X2, value1, value2, value3);
	assert_dup2_x2_stack(OPC_DUP2_X2, value1, value2, value3, value4);

	expr_put(value1);
	expr_put(value2);
	expr_put(value3);
	expr_put(value4);
}

static void
assert_swap_stack(unsigned char opc, void *value1, void *value2)
{
	struct basic_block *bb;

	bb = alloc_simple_bb(&opc, 1);

	stack_push(bb->mimic_stack, expr_get(value1));
	stack_push(bb->mimic_stack, expr_get(value2));

	convert_to_ir(bb->b_parent);
	assert_ptr_equals(value1, pop_and_put_expr(bb->mimic_stack));
	assert_ptr_equals(value2, pop_and_put_expr(bb->mimic_stack));
	assert_true(stack_is_empty(bb->mimic_stack));

	free_simple_bb(bb);
}

void test_convert_swap(void)
{
	struct expression *expr1;
	struct expression *expr2;

	expr1 = value_expr(J_INT, 1);
	expr2 = value_expr(J_INT, 2);

	assert_swap_stack(OPC_SWAP, expr1, expr2);

	expr_put(expr1);
	expr_put(expr2);
}
