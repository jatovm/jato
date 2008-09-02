/*
 * Copyright (C) 2005-2006  Pekka Enberg
 */

#include <vm/vm.h>
#include <jit/expression.h>
#include <jit/compiler.h>
#include <libharness.h>

#include "bc-test-utils.h"

static void assert_conversion_expr_stack(unsigned char opc,
					 enum vm_type from_type,
					 enum vm_type to_type)
{
	unsigned char code[] = { opc };
	struct expression *expression, *conversion_expression;
	struct compilation_unit *cu;
	struct methodblock method = {
		.jit_code = code,
		.code_size = ARRAY_SIZE(code),
	};
	struct var_info *temporary;

	cu = alloc_simple_compilation_unit(&method);

	temporary = get_var(cu);
	expression = temporary_expr(from_type, NULL, temporary);
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
