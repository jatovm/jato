/*
 * Copyright (C) 2005-2006  Pekka Enberg
 */

#include <bc-test-utils.h>
#include <args-test-utils.h>
#include <jit/compilation-unit.h>
#include <jit/expression.h>
#include <jit/compiler.h>
#include <libharness.h>
#include <vm/stack.h>
#include <vm/system.h>
#include <vm/types.h>
#include <vm/vm.h>

#include <test/vm.h>

#include <string.h>

static void convert_ir_const_single(struct compilation_unit *cu, void *value, uint8_t type)
{
	uint64_t cp_infos[] = { (unsigned long) value };
	uint8_t cp_types[] = { type };

	convert_ir_const(cu, (void *)cp_infos, 8, cp_types);
}

struct type_mapping {
	char *type;
	enum vm_type vm_type;
};

static struct type_mapping types[] = {
	{ "B", J_BYTE },
	{ "C", J_CHAR },
	{ "D", J_DOUBLE },
	{ "F", J_FLOAT },
	{ "I", J_INT },
	{ "J", J_LONG },
	{ "L/java/long/Object", J_REFERENCE },
	{ "S", J_SHORT },
	{ "Z", J_BOOLEAN },
};

static void __assert_convert_getstatic(unsigned char opc,
				       enum vm_type expected_vm_type,
				       char *field_type)
{
	struct vm_field fb;
	struct expression *expr;
	unsigned char code[] = { opc, 0x00, 0x00 };
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};
	struct basic_block *bb;

	fb.type = field_type;

	bb = __alloc_simple_bb(&method);

	convert_ir_const_single(bb->b_parent, &fb, CAFEBABE_CONSTANT_TAG_FIELD_REF);
	expr = stack_pop(bb->mimic_stack);
	assert_class_field_expr(expected_vm_type, &fb, &expr->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	__free_simple_bb(bb);
}

static void assert_convert_getstatic(unsigned char opc)
{
	unsigned long i;

	for (i = 0; i < ARRAY_SIZE(types); i++)
		__assert_convert_getstatic(opc, types[i].vm_type, types[i].type);
}

void test_convert_getstatic(void)
{
	NOT_IMPLEMENTED;

	//assert_convert_getstatic(OPC_GETSTATIC);
}

static void __assert_convert_getfield(unsigned char opc,
				      enum vm_type expected_vm_type,
				      char *field_type)
{
	struct vm_field fb;
	struct expression *expr;
	struct expression *objectref;
	unsigned char code[] = { opc, 0x00, 0x00 };
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};
	struct basic_block *bb;

	fb.type = field_type;

	bb = __alloc_simple_bb(&method);

	objectref = value_expr(J_REFERENCE, 0xdeadbeef);
	stack_push(bb->mimic_stack, objectref);
	convert_ir_const_single(bb->b_parent, &fb, CAFEBABE_CONSTANT_TAG_FIELD_REF);
	expr = stack_pop(bb->mimic_stack);
	assert_instance_field_expr(expected_vm_type, &fb, objectref, &expr->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(expr);
	__free_simple_bb(bb);
}


static void assert_convert_getfield(unsigned char opc)
{
	unsigned long i;

	for (i = 0; i < ARRAY_SIZE(types); i++)
		__assert_convert_getfield(opc, types[i].vm_type, types[i].type);
}

void test_convert_getfield(void)
{
	NOT_IMPLEMENTED;

	//assert_convert_getfield(OPC_GETFIELD);
}

static void __assert_convert_putstatic(unsigned char opc,
				       enum vm_type expected_vm_type,
				       char *field_type)
{
	struct vm_field fb;
	struct statement *stmt;
	unsigned char code[] = { opc, 0x00, 0x00 };
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};
	struct basic_block *bb;
	struct expression *value;

	fb.type = field_type;
	value = value_expr(expected_vm_type, 0xdeadbeef);
	bb = __alloc_simple_bb(&method);
	stack_push(bb->mimic_stack, value);
	convert_ir_const_single(bb->b_parent, &fb, CAFEBABE_CONSTANT_TAG_FIELD_REF);
	stmt = stmt_entry(bb->stmt_list.next);

	assert_store_stmt(stmt);
	assert_class_field_expr(expected_vm_type, &fb, stmt->store_dest);
	assert_ptr_equals(value, to_expr(stmt->store_src));
	assert_true(stack_is_empty(bb->mimic_stack));

	__free_simple_bb(bb);
}

static void assert_convert_putstatic(unsigned char opc)
{
	unsigned long i;

	for (i = 0; i < ARRAY_SIZE(types); i++)
		__assert_convert_putstatic(opc, types[i].vm_type, types[i].type);
}

void test_convert_putstatic(void)
{
	NOT_IMPLEMENTED;

	//assert_convert_putstatic(OPC_PUTSTATIC);
}

static void __assert_convert_putfield(unsigned char opc,
				      enum vm_type expected_vm_type,
				      char *field_type)
{
	struct vm_field fb;
	struct statement *stmt;
	unsigned char code[] = { opc, 0x00, 0x00 };
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};
	struct basic_block *bb;
	struct expression *objectref;
	struct expression *value;

	fb.type = field_type;
	bb = __alloc_simple_bb(&method);

	objectref = value_expr(J_REFERENCE, 0xdeadbeef);
	stack_push(bb->mimic_stack, objectref);

	value = value_expr(expected_vm_type, 0xdeadbeef);
	stack_push(bb->mimic_stack, value);

	convert_ir_const_single(bb->b_parent, &fb, CAFEBABE_CONSTANT_TAG_FIELD_REF);
	stmt = stmt_entry(bb->stmt_list.next);

	assert_store_stmt(stmt);
	assert_instance_field_expr(expected_vm_type, &fb, objectref, stmt->store_dest);
	assert_ptr_equals(value, to_expr(stmt->store_src));
	assert_true(stack_is_empty(bb->mimic_stack));

	__free_simple_bb(bb);
}

static void assert_convert_putfield(unsigned char opc)
{
	unsigned long i;

	for (i = 0; i < ARRAY_SIZE(types); i++)
		__assert_convert_putfield(opc, types[i].vm_type, types[i].type);
}

void test_convert_putfield(void)
{
	NOT_IMPLEMENTED;

	//assert_convert_putfield(OPC_PUTFIELD);
}

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
	assert_temporary_expr(arrayref_pure_stmt->store_dest);

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

	assert_temporary_expr(&temporary_expr->node);
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
	struct var_info *temporary;

	bb = __alloc_simple_bb(&method);

	arrayref_expr = value_expr(J_REFERENCE, arrayref);
	index_expr = value_expr(J_INT, index);
	temporary = get_var(bb->b_parent);
	expr = temporary_expr(expected_type, NULL, temporary);

	stack_push(bb->mimic_stack, arrayref_expr);
	stack_push(bb->mimic_stack, index_expr);
	stack_push(bb->mimic_stack, expr);

	convert_to_ir(bb->b_parent);
	stmt = stmt_entry(bb->stmt_list.next);

	struct statement *arrayref_pure_stmt = stmt;
	struct statement *value_dup_stmt = stmt_entry(arrayref_pure_stmt->stmt_list_node.next);
	struct statement *arraycheck_stmt = stmt_entry(value_dup_stmt->stmt_list_node.next);
	struct statement *storecheck_stmt = stmt_entry(arraycheck_stmt->stmt_list_node.next);
	struct statement *store_stmt = stmt_entry(storecheck_stmt->stmt_list_node.next);

	assert_store_stmt(arrayref_pure_stmt);
	assert_nullcheck_value_expr(J_REFERENCE, arrayref,
				    arrayref_pure_stmt->store_src);
	assert_temporary_expr(arrayref_pure_stmt->store_dest);

	assert_store_stmt(value_dup_stmt);
	assert_ptr_equals(&expr->node, value_dup_stmt->store_src);
	assert_temporary_expr(value_dup_stmt->store_dest);

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
	assert_temporary_expr(store_stmt->store_src);

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

static void assert_convert_new(unsigned long expected_type_idx,
			       unsigned char idx_1, unsigned char idx_2)
{
	struct object *instance_class = new_class();
	unsigned char code[] = { OPC_NEW, 0x0, 0x0 };
	struct basic_block *bb;
	struct expression *new_expr;
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};

	bb = __alloc_simple_bb(&method);
	convert_ir_const_single(bb->b_parent, instance_class, CAFEBABE_CONSTANT_TAG_CLASS);

	new_expr = stack_pop(bb->mimic_stack);
	assert_int_equals(EXPR_NEW, expr_type(new_expr));
	assert_int_equals(J_REFERENCE, new_expr->vm_type);
	assert_ptr_equals(instance_class, new_expr->class);

	free_expression(new_expr);
	__free_simple_bb(bb);

	free(instance_class);
}

void test_convert_new(void)
{
	NOT_IMPLEMENTED;

	//assert_convert_new(0xcafe, 0xca, 0xfe);
}

void test_convert_anewarray(void)
{
	NOT_IMPLEMENTED;

#if 0
	struct object *instance_class = new_class();
	unsigned char code[] = { OPC_ANEWARRAY, 0x00, 0x00 };
	struct basic_block *bb;
	char array_name[] = "java/lang/Object";
	struct expression *size,*arrayref;
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};

	bb = __alloc_simple_bb(&method);
	size = value_expr(J_INT, 0xff);
	stack_push(bb->mimic_stack, size);

	CLASS_CB(instance_class)->name = array_name;

	convert_ir_const_single(bb->b_parent, instance_class, CAFEBABE_CONSTANT_TAG_CLASS);

	arrayref = stack_pop(bb->mimic_stack);

	assert_int_equals(EXPR_ANEWARRAY, expr_type(arrayref));
	assert_int_equals(J_REFERENCE, arrayref->vm_type);
	assert_array_size_check_expr(size, to_expr(arrayref->anewarray_size));

        /*
	 * Free the struct object returned from findArrayClassFromClassLoader()
	 * stub.
         */
	free(arrayref->anewarray_ref_type);

	expr_put(arrayref);
	__free_simple_bb(bb);
	free(instance_class);
#endif
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

void test_convert_multianewarray(void)
{
	NOT_IMPLEMENTED;

#if 0
	struct object *instance_class = new_class();
	unsigned char dimension = 0x02;
	unsigned char code[] = { OPC_MULTIANEWARRAY, 0x00, 0x00, dimension };
	struct expression *arrayref;
	struct expression *args_count[dimension];
	struct basic_block *bb;
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};

	bb = __alloc_simple_bb(&method);

	create_args(args_count, dimension);
	push_args(bb, args_count, dimension);

	convert_ir_const_single(bb->b_parent, instance_class, CAFEBABE_CONSTANT_TAG_CLASS);

	arrayref = stack_pop(bb->mimic_stack);

	assert_int_equals(EXPR_MULTIANEWARRAY, expr_type(arrayref));
	assert_int_equals(J_REFERENCE, arrayref->vm_type);
	assert_ptr_equals(instance_class, arrayref->multianewarray_ref_type);

	assert_multiarray_size_check_expr(args_count, dimension,
		to_expr(arrayref->multianewarray_dimensions));

	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(arrayref);
	__free_simple_bb(bb);
	free(instance_class);
#endif
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
	struct object *class = new_class();

	bb = __alloc_simple_bb(&method);

	arrayref = value_expr(J_REFERENCE, (unsigned long) class);
	stack_push(bb->mimic_stack, arrayref);

	convert_to_ir(bb->b_parent);

	arraylen_exp = stack_pop(bb->mimic_stack);
	assert_int_equals(EXPR_ARRAYLENGTH, expr_type(arraylen_exp));
	assert_int_equals(J_REFERENCE, arraylen_exp->vm_type);
	assert_ptr_equals(arrayref, to_expr(arraylen_exp->arraylength_ref));

	expr_put(arraylen_exp);
	__free_simple_bb(bb);
	free(class);
}

void test_convert_instanceof(void)
{
	NOT_IMPLEMENTED;

#if 0
	struct object *instance_class = new_class();
	unsigned char code[] = { OPC_INSTANCEOF, 0x00, 0x00 };
	struct basic_block *bb;
	struct expression *ref, *expr;
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};

	bb = __alloc_simple_bb(&method);
	ref = value_expr(J_REFERENCE, (unsigned long) instance_class);
	stack_push(bb->mimic_stack, ref);

	convert_ir_const_single(bb->b_parent, instance_class, CAFEBABE_CONSTANT_TAG_CLASS);

	expr = stack_pop(bb->mimic_stack);

	assert_int_equals(EXPR_INSTANCEOF, expr_type(expr));
	assert_int_equals(J_REFERENCE, expr->vm_type);
	assert_ptr_equals(ref, to_expr(expr->instanceof_ref));
	assert_ptr_equals(instance_class, expr->instanceof_class);

	expr_put(expr);
	__free_simple_bb(bb);
	free(instance_class);
#endif
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
	struct object *class = new_class();

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
	struct object *class = new_class();

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

void test_convert_checkcast(void)
{
	NOT_IMPLEMENTED;

#if 0
	unsigned char code[] = {OPC_CHECKCAST, 0x00, 0x00};
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};

	struct expression *ref;
	struct statement *stmt;
	struct basic_block *bb;
	struct object *class = new_class();

	bb = __alloc_simple_bb(&method);

	ref = value_expr(J_REFERENCE, (unsigned long)class);
	stack_push(bb->mimic_stack, ref);

	convert_ir_const_single(bb->b_parent, class, CAFEBABE_CONSTANT_TAG_CLASS);
	stmt = stmt_entry(bb->stmt_list.next);
	assert_checkcast_stmt(ref, stmt);

	expr_put(ref);
	__free_simple_bb(bb);
	free(class);
#endif
}
