/*
 * Copyright (C) 2005-2006  Pekka Enberg
 */

#include <bc-test-utils.h>
#include <compilation-unit.h>
#include <jit/expression.h>
#include <jvm_types.h>
#include <libharness.h>
#include <vm/stack.h>
#include <vm/system.h>
#include <vm/vm.h>

static void convert_ir_field(struct compilation_unit *cu, struct fieldblock *fb)
{
	u8 cp_infos[] = { (unsigned long) fb };
	u1 cp_types[] = { CONSTANT_Resolved };

	convert_ir_const(cu, (void *)cp_infos, 8, cp_types);
}

static void assert_convert_getstatic(enum jvm_type expected_jvm_type,
				     char *field_type)
{
	struct fieldblock fb;
	struct expression *expr;
	unsigned char code[] = { OPC_GETSTATIC, 0x00, 0x00 };
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct compilation_unit *cu;

	fb.type = field_type;

	cu = alloc_simple_compilation_unit(&method);

	convert_ir_field(cu, &fb);
	expr = stack_pop(cu->expr_stack);
	assert_field_expr(expected_jvm_type, &fb, &expr->node);
	assert_true(stack_is_empty(cu->expr_stack));

	expr_put(expr);
	free_compilation_unit(cu);
}

void test_convert_getstatic(void)
{
	assert_convert_getstatic(J_BYTE, "B");
	assert_convert_getstatic(J_CHAR, "C");
	assert_convert_getstatic(J_DOUBLE, "D");
	assert_convert_getstatic(J_FLOAT, "F");
	assert_convert_getstatic(J_INT, "I");
	assert_convert_getstatic(J_LONG, "J");
	assert_convert_getstatic(J_REFERENCE, "Ljava/lang/Object;");
	assert_convert_getstatic(J_SHORT, "S");
	assert_convert_getstatic(J_BOOLEAN, "Z");
}

static void assert_convert_putstatic(enum jvm_type expected_jvm_type,
				     char *field_type)
{
	struct fieldblock fb;
	struct statement *stmt;
	unsigned char code[] = { OPC_PUTSTATIC, 0x00, 0x00 };
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct compilation_unit *cu;
	struct expression *value;

	fb.type = field_type;
	value = value_expr(expected_jvm_type, 0xdeadbeef);
	cu = alloc_simple_compilation_unit(&method);
	stack_push(cu->expr_stack, value);
	convert_ir_field(cu, &fb);
	stmt = stmt_entry(bb_entry(cu->bb_list.next)->stmt_list.next);

	assert_store_stmt(stmt);
	assert_field_expr(expected_jvm_type, &fb, stmt->store_dest);
	assert_ptr_equals(value, to_expr(stmt->store_src));
	assert_true(stack_is_empty(cu->expr_stack));

	free_compilation_unit(cu);
}

void test_convert_putstatic(void)
{
	assert_convert_putstatic(J_BYTE, "B");
	assert_convert_putstatic(J_CHAR, "C");
	assert_convert_putstatic(J_DOUBLE, "D");
	assert_convert_putstatic(J_FLOAT, "F");
	assert_convert_putstatic(J_INT, "I");
	assert_convert_putstatic(J_LONG, "J");
	assert_convert_putstatic(J_REFERENCE, "Ljava/lang/Object;");
	assert_convert_putstatic(J_SHORT, "S");
	assert_convert_putstatic(J_BOOLEAN, "Z");
}
