/*
 * Copyright (C) 2005-2006  Pekka Enberg
 */

#include <bc-test-utils.h>
#include <args-test-utils.h>
#include "jit/compilation-unit.h"
#include "jit/expression.h"
#include "jit/compiler.h"
#include <libharness.h>
#include "lib/stack.h"
#include "vm/system.h"
#include "vm/types.h"
#include "vm/vm.h"

#include "test/vm.h"

#include <string.h>

struct type_mapping {
	char *type;
	enum vm_type vm_type;
};

static void assert_convert_array_load(enum vm_type expected_type,
				      unsigned char opc,
				      unsigned long arrayref,
				      unsigned long index)
{
	unsigned char code[] = { opc };
	struct expression *arrayref_expr, *index_expr, *temporary_expr;
	struct statement *stmt;
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};
	struct basic_block *bb;

	bb = __alloc_simple_bb(&method);

	arrayref_expr = value_expr(J_REFERENCE, arrayref);
	index_expr = value_expr(J_INT, index);

	stack_push(bb->mimic_stack, arrayref_expr);
	stack_push(bb->mimic_stack, index_expr);

	convert_to_ir(bb->b_parent);
	stmt = stmt_entry(bb->stmt_list.next);

	struct statement *arrayref_pure_stmt = stmt;
	struct statement *arraycheck_stmt = stmt_entry(arrayref_pure_stmt->stmt_list_node.next);
	struct statement *store_stmt = stmt_entry(arraycheck_stmt->stmt_list_node.next);

	assert_store_stmt(arrayref_pure_stmt);
	assert_nullcheck_value_expr(J_REFERENCE, arrayref,
				    arrayref_pure_stmt->store_src);
	assert_temporary_expr(J_REFERENCE, arrayref_pure_stmt->store_dest);

	assert_arraycheck_stmt(expected_type,
			       to_expr(arrayref_pure_stmt->store_dest),
			       index_expr,
			       arraycheck_stmt);

	assert_store_stmt(store_stmt);
	assert_array_deref_expr(expected_type,
				to_expr(arrayref_pure_stmt->store_dest),
				index_expr,
				store_stmt->store_src);

	temporary_expr = stack_pop(bb->mimic_stack);

	assert_temporary_expr(expected_type, &temporary_expr->node);
	expr_put(temporary_expr);
	assert_true(stack_is_empty(bb->mimic_stack));

	__free_simple_bb(bb);
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
	assert_convert_array_load(J_BYTE, OPC_BALOAD, 0, 1);
	assert_convert_array_load(J_BYTE, OPC_BALOAD, 1, 2);
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

static void assert_convert_array_store(enum vm_type expected_type,
				       unsigned char opc,
				       unsigned long arrayref,
				       unsigned long index)
{
	unsigned char code[] = { opc };
	struct expression *arrayref_expr, *index_expr, *expr;
	struct statement *stmt;
	struct basic_block *bb;
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};

	bb = __alloc_simple_bb(&method);

	arrayref_expr = value_expr(J_REFERENCE, arrayref);
	index_expr = value_expr(J_INT, index);
	expr = temporary_expr(expected_type, bb->b_parent);

	stack_push(bb->mimic_stack, arrayref_expr);
	stack_push(bb->mimic_stack, index_expr);
	stack_push(bb->mimic_stack, expr);

	convert_to_ir(bb->b_parent);
	stmt = stmt_entry(bb->stmt_list.next);

	struct statement *arrayref_pure_stmt = stmt;
	struct statement *arraycheck_stmt = stmt_entry(arrayref_pure_stmt->stmt_list_node.next);
	struct statement *storecheck_stmt = stmt_entry(arraycheck_stmt->stmt_list_node.next);
	struct statement *store_stmt = stmt_entry(storecheck_stmt->stmt_list_node.next);

	assert_store_stmt(arrayref_pure_stmt);
	assert_nullcheck_value_expr(J_REFERENCE, arrayref,
				    arrayref_pure_stmt->store_src);
	assert_temporary_expr(J_REFERENCE, arrayref_pure_stmt->store_dest);

	assert_arraycheck_stmt(expected_type,
			       to_expr(arrayref_pure_stmt->store_dest),
			       index_expr,
			       arraycheck_stmt);

	assert_array_store_check_stmt(storecheck_stmt,
				      to_expr(arrayref_pure_stmt->store_dest),
				      store_stmt->store_src);
	assert_store_stmt(store_stmt);
	assert_array_deref_expr(expected_type,
				to_expr(arrayref_pure_stmt->store_dest),
				index_expr,
				store_stmt->store_dest);
	assert_temporary_expr(expected_type, store_stmt->store_src);

	assert_true(stack_is_empty(bb->mimic_stack));

	__free_simple_bb(bb);
}

void test_convert_iastore(void)
{
	assert_convert_array_store(J_INT, OPC_IASTORE, 0, 1);
	assert_convert_array_store(J_INT, OPC_IASTORE, 2, 3);
}

void test_convert_lastore(void)
{
	assert_convert_array_store(J_LONG, OPC_LASTORE, 0, 1);
	assert_convert_array_store(J_LONG, OPC_LASTORE, 2, 3);
}

void test_convert_fastore(void)
{
	assert_convert_array_store(J_FLOAT, OPC_FASTORE, 0, 1);
	assert_convert_array_store(J_FLOAT, OPC_FASTORE, 2, 3);
}

void test_convert_dastore(void)
{
	assert_convert_array_store(J_DOUBLE, OPC_DASTORE, 0, 1);
	assert_convert_array_store(J_DOUBLE, OPC_DASTORE, 2, 3);
}

void test_convert_aastore(void)
{
	assert_convert_array_store(J_REFERENCE, OPC_AASTORE, 0, 1);
	assert_convert_array_store(J_REFERENCE, OPC_AASTORE, 2, 3);
}

void test_convert_bastore(void)
{
	assert_convert_array_store(J_BYTE, OPC_BASTORE, 0, 1);
	assert_convert_array_store(J_BYTE, OPC_BASTORE, 2, 3);
}

void test_convert_castore(void)
{
	assert_convert_array_store(J_CHAR, OPC_CASTORE, 0, 1);
	assert_convert_array_store(J_CHAR, OPC_CASTORE, 2, 3);
}

void test_convert_sastore(void)
{
	assert_convert_array_store(J_SHORT, OPC_SASTORE, 0, 1);
	assert_convert_array_store(J_SHORT, OPC_SASTORE, 2, 3);
}

void test_convert_newarray(void)
{
	unsigned char code[] = { OPC_NEWARRAY, T_INT };
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};
	struct expression *size, *arrayref;
	struct basic_block *bb;

	bb = __alloc_simple_bb(&method);

	size = value_expr(J_INT, 0xff);
	stack_push(bb->mimic_stack, size);

	convert_to_ir(bb->b_parent);

	arrayref = stack_pop(bb->mimic_stack);
	assert_int_equals(EXPR_NEWARRAY, expr_type(arrayref));
	assert_int_equals(J_REFERENCE, arrayref->vm_type);
	assert_array_size_check_expr(size, to_expr(arrayref->array_size));
	assert_int_equals(T_INT, arrayref->array_type);

	expr_put(arrayref);
	__free_simple_bb(bb);
}

void test_convert_arraylength(void)
{
	unsigned char code[] = { OPC_ARRAYLENGTH };
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};
	struct expression *arrayref, *arraylen_exp;
	struct basic_block *bb;
	struct vm_class *class = new_class();

	bb = __alloc_simple_bb(&method);

	arrayref = value_expr(J_REFERENCE, (unsigned long) class);
	stack_push(bb->mimic_stack, arrayref);

	convert_to_ir(bb->b_parent);

	arraylen_exp = stack_pop(bb->mimic_stack);
	assert_int_equals(EXPR_ARRAYLENGTH, expr_type(arraylen_exp));
	assert_int_equals(J_INT, arraylen_exp->vm_type);
	assert_nullcheck_value_expr(J_REFERENCE, (unsigned long) class, arraylen_exp->arraylength_ref);

	expr_put(arraylen_exp);
	__free_simple_bb(bb);
	free(class);
}

void test_convert_monitorenter(void)
{
	unsigned char code[] = { OPC_MONITORENTER };
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};
	struct expression *ref;
	struct statement *stmt;
	struct basic_block *bb;
	struct vm_class *class = new_class();

	bb = __alloc_simple_bb(&method);

	ref = value_expr(J_REFERENCE, (unsigned long) class);
	stack_push(bb->mimic_stack, ref);

	convert_to_ir(bb->b_parent);
	stmt = stmt_entry(bb->stmt_list.next);

	assert_monitorenter_stmt(ref, stmt);

	expr_put(ref);
	__free_simple_bb(bb);
	free(class);
}

void test_convert_monitorexit(void)
{
	unsigned char code[] = { OPC_MONITOREXIT };
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};
	struct expression *ref;
	struct statement *stmt;
	struct basic_block *bb;
	struct vm_class *class = new_class();

	bb = __alloc_simple_bb(&method);

	ref = value_expr(J_REFERENCE, (unsigned long) class);
	stack_push(bb->mimic_stack, ref);

	convert_to_ir(bb->b_parent);
	stmt = stmt_entry(bb->stmt_list.next);

	assert_monitorexit_stmt(ref, stmt);

	expr_put(ref);
	__free_simple_bb(bb);
	free(class);
}
