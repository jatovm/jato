/*
 * Copyright (C) 2005-2006  Pekka Enberg
 */

#include "cafebabe/constant_pool.h"
#include "cafebabe/class.h"

#include "jit/compilation-unit.h"
#include "jit/basic-block.h"
#include <bc-test-utils.h>
#include <args-test-utils.h>
#include "jit/tree-node.h"
#include "jit/expression.h"
#include "jit/statement.h"
#include "jit/compiler.h"
#include "vm/types.h"
#include <libharness.h>
#include <stdlib.h>

#include "test/vm.h"

struct compilation_unit *
alloc_simple_compilation_unit(struct vm_method *method)
{
	struct compilation_unit *cu;
	struct basic_block *bb;

	cu = compilation_unit_alloc(method);
	bb = get_basic_block(cu, 0, method->code_attribute.code_length);
	cu->entry_bb = bb;

	return cu;
}

struct basic_block *__alloc_simple_bb(struct vm_method *method)
{
	struct compilation_unit *cu;
	struct basic_block *bb;

	cu = compilation_unit_alloc(method);
	bb = get_basic_block(cu, 0, method->code_attribute.code_length);
	cu->entry_bb = bb;

	return bb;
}

struct basic_block *
alloc_simple_bb(unsigned char *code, unsigned long code_size)
{
	struct vm_method *method;

	method = malloc(sizeof *method);
	method->code_attribute.code = code;
	method->code_attribute.code_length = code_size;
	method->args_count = 0;
	method->code_attribute.max_locals = 0;

	return __alloc_simple_bb(method);
}

void __free_simple_bb(struct basic_block *bb)
{
	free_compilation_unit(bb->b_parent);
}

void free_simple_bb(struct basic_block *bb)
{
	struct compilation_unit *cu = bb->b_parent;

	free(cu->method);
	__free_simple_bb(bb);
}

void assert_value_expr(enum vm_type expected_vm_type,
		       long long expected_value, struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	assert_int_equals(EXPR_VALUE, expr_type(expr));
	assert_int_equals(expected_vm_type, expr->vm_type);
	assert_int_equals(expected_value, expr->value);
}

void assert_nullcheck_value_expr(enum vm_type expected_vm_type,
				 long long expected_value,
				 struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	assert_int_equals(EXPR_NULL_CHECK, expr_type(expr));
	assert_value_expr(expected_vm_type, expected_value,
			  expr->null_check_ref);
}

void assert_fvalue_expr(enum vm_type expected_vm_type,
			double expected_value, struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	assert_int_equals(EXPR_FVALUE, expr_type(expr));
	assert_int_equals(expected_vm_type, expr->vm_type);
	assert_float_equals(expected_value, expr->fvalue, 0.01f);
}

void assert_local_expr(enum vm_type expected_vm_type,
		       unsigned long expected_index, struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	assert_int_equals(EXPR_LOCAL, expr_type(expr));
	assert_int_equals(expected_vm_type, expr->vm_type);
	assert_int_equals(expected_index, expr->local_index);
}

void assert_temporary_expr(struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	assert_int_equals(EXPR_TEMPORARY, expr_type(expr));
}

void assert_null_check_expr(struct expression *expected,
			    struct expression *actual)
{
	assert_int_equals(EXPR_NULL_CHECK, expr_type(actual));
	assert_ptr_equals(expected, to_expr(actual->null_check_ref));
}

void assert_array_deref_expr(enum vm_type expected_vm_type,
			     struct expression *expected_arrayref,
			     struct expression *expected_index,
			     struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	assert_int_equals(EXPR_ARRAY_DEREF, expr_type(expr));
	assert_int_equals(expected_vm_type, expr->vm_type);
	assert_ptr_equals(expected_arrayref, to_expr(expr->arrayref));
	assert_ptr_equals(expected_index, to_expr(expr->array_index));
}

void __assert_binop_expr(enum vm_type vm_type,
			 enum binary_operator binary_operator,
			 struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	assert_int_equals(EXPR_BINOP, expr_type(expr));
	assert_int_equals(vm_type, expr->vm_type);
	assert_int_equals(binary_operator, expr_bin_op(expr));
}

void assert_binop_expr(enum vm_type vm_type,
		       enum binary_operator binary_operator,
		       struct expression *binary_left,
		       struct expression *binary_right, struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	__assert_binop_expr(vm_type, binary_operator, node);
	assert_ptr_equals(binary_left, to_expr(expr->binary_left));
	assert_ptr_equals(binary_right, to_expr(expr->binary_right));
}

void assert_conv_expr(enum vm_type expected_type,
		      enum expression_type expected_expr_type,
		      struct expression *expected_expression,
		      struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	assert_int_equals(expected_expr_type, expr_type(expr));
	assert_int_equals(expected_type, expr->vm_type);
	assert_ptr_equals(expected_expression, to_expr(expr->from_expression));
}

static void __assert_field_expr(enum expression_type expected_expr_type,
				enum vm_type expected_type,
				struct expression *expr)
{
	assert_int_equals(expected_expr_type, expr_type(expr));
	assert_int_equals(expected_type, expr->vm_type);
}

void assert_class_field_expr(enum vm_type expected_vm_type,
			     struct vm_field *expected_field,
			     struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	__assert_field_expr(EXPR_CLASS_FIELD, expected_vm_type, expr);
	assert_ptr_equals(expected_field, expr->class_field);
}

void assert_instance_field_expr(enum vm_type expected_vm_type,
				struct vm_field *expected_field,
				struct expression *expected_objectref,
				struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	__assert_field_expr(EXPR_INSTANCE_FIELD, expected_vm_type, expr);
	assert_ptr_equals(expected_field, expr->instance_field);
	assert_ptr_equals(expected_objectref, to_expr(expr->objectref_expression));
}

void assert_invoke_expr(enum vm_type expected_type,
			struct vm_method *expected_method,
			struct tree_node *node)
{
	struct expression *expr = to_expr(node);

	assert_int_equals(EXPR_INVOKE, expr_type(expr));
	assert_int_equals(expected_type, expr->vm_type);
	assert_ptr_equals(expected_method, expr->target_method);
}

void assert_array_size_check_expr(struct expression *expected,
				  struct expression *actual)
{
	assert_int_equals(EXPR_ARRAY_SIZE_CHECK, expr_type(actual));
	assert_ptr_equals(&expected->node, actual->size_expr);
}

void assert_multiarray_size_check_expr(struct expression **expected_args,
				       int nr_args,
				       struct expression *actual)
{
	assert_int_equals(EXPR_MULTIARRAY_SIZE_CHECK, expr_type(actual));
	assert_args(expected_args, nr_args, to_expr(actual->size_expr));
}

void assert_store_stmt(struct statement *stmt)
{
	assert_int_equals(STMT_STORE, stmt_type(stmt));
}

void assert_array_store_check_stmt(struct statement *stmt,
				   struct expression *arrayref,
				   struct tree_node *store_src)
{
	assert_int_equals(STMT_ARRAY_STORE_CHECK, stmt_type(stmt));
	assert_ptr_equals(arrayref, to_expr(stmt->store_check_array));
	assert_ptr_equals(store_src, stmt->store_check_src);
}

void assert_return_stmt(struct expression *return_value, struct statement *stmt)
{
	assert_int_equals(STMT_RETURN, stmt_type(stmt));
	assert_ptr_equals(return_value, to_expr(stmt->return_value));
}

void assert_void_return_stmt(struct statement *stmt)
{
	assert_int_equals(STMT_VOID_RETURN, stmt_type(stmt));
	assert_ptr_equals(NULL, stmt->return_value);
}

void assert_arraycheck_stmt(enum vm_type expected_vm_type,
			    struct expression *expected_arrayref,
			    struct expression *expected_index,
			    struct statement *actual)
{
	assert_int_equals(STMT_ARRAY_CHECK, stmt_type(actual));
	assert_array_deref_expr(expected_vm_type, expected_arrayref,
				expected_index, actual->expression);
}

void assert_monitorenter_stmt(struct expression *expected,
			       struct statement *actual)
{
	assert_int_equals(STMT_MONITOR_ENTER, stmt_type(actual));
	assert_nullcheck_value_expr(J_REFERENCE, expected->value,
				    actual->expression);
}

void assert_monitorexit_stmt(struct expression *expected,
			      struct statement *actual)
{
	assert_int_equals(STMT_MONITOR_EXIT, stmt_type(actual));
	assert_nullcheck_value_expr(J_REFERENCE, expected->value,
				    actual->expression);
}

void assert_checkcast_stmt(struct expression *expected,
			   struct statement *actual)
{
	assert_int_equals(STMT_CHECKCAST, stmt_type(actual));
	assert_value_expr(J_REFERENCE, expected->value, actual->expression);
}

void convert_ir_const(struct compilation_unit *cu,
		      uint32_t *cp_infos,
		      size_t nr_cp_infos, uint8_t *cp_types)
{
	struct vm_class *class = new_class();
	struct cafebabe_class *cb = class->class;
	unsigned i;

	for (i = 0; i < nr_cp_infos; i++) {
		struct cafebabe_constant_pool *cp = &cb->constant_pool[i];

		cp->long_ = *(struct cafebabe_constant_info_long *) &cp_infos[i];
		cp->tag = cp_types[i];
	}

	cb->constant_pool_count = nr_cp_infos;

	cu->method->class = class;
	convert_to_ir(cu);

	free(class);
}

struct statement *first_stmt(struct compilation_unit *cu)
{
	return stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);
}
