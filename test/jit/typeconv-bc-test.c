/*
 * Copyright (C) 2005-2006  Pekka Enberg
 */

#include "vm/vm.h"
#include "jit/expression.h"
#include "jit/compiler.h"
#include <libharness.h>

#include "bc-test-utils.h"

static void assert_conversion_mimic_stack(unsigned char opc,
					 enum expression_type expr_type,
					 enum vm_type from_type,
					 enum vm_type to_type)
{
	unsigned char code[] = { opc };
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};
	struct expression *conversion_expression;
	struct expression *expression;
	struct var_info *temporary;
	struct basic_block *bb;

	bb = __alloc_simple_bb(&method);
	temporary = get_var(bb->b_parent, J_INT);
	expression = temporary_expr(from_type, NULL, temporary);
	stack_push(bb->mimic_stack, expression);
	convert_to_ir(bb->b_parent);

	conversion_expression = stack_pop(bb->mimic_stack);
	assert_conv_expr(to_type, expr_type, expression, &conversion_expression->node);
	assert_true(stack_is_empty(bb->mimic_stack));

	expr_put(conversion_expression);
	__free_simple_bb(bb);
}

void test_convert_int_widening(void)
{
	assert_conversion_mimic_stack(OPC_I2L, EXPR_CONVERSION, J_INT, J_LONG);
	assert_conversion_mimic_stack(OPC_I2F, EXPR_CONVERSION_TO_FLOAT, J_INT, J_FLOAT);
	assert_conversion_mimic_stack(OPC_I2D, EXPR_CONVERSION_TO_DOUBLE, J_INT, J_DOUBLE);
}

void test_convert_long_conversion(void)
{
	assert_conversion_mimic_stack(OPC_L2I, EXPR_CONVERSION, J_LONG, J_INT);
	assert_conversion_mimic_stack(OPC_L2F, EXPR_CONVERSION_TO_FLOAT, J_LONG, J_FLOAT);
	assert_conversion_mimic_stack(OPC_L2D, EXPR_CONVERSION_TO_DOUBLE, J_LONG, J_DOUBLE);
}

void test_convert_float_conversion(void)
{
	assert_conversion_mimic_stack(OPC_F2I, EXPR_CONVERSION_FROM_FLOAT, J_FLOAT, J_INT);
	assert_conversion_mimic_stack(OPC_F2L, EXPR_CONVERSION_FROM_FLOAT, J_FLOAT, J_LONG);
	assert_conversion_mimic_stack(OPC_F2D, EXPR_CONVERSION_FLOAT_TO_DOUBLE, J_FLOAT, J_DOUBLE);
}

void test_convert_double_conversion(void)
{
	assert_conversion_mimic_stack(OPC_D2I, EXPR_CONVERSION_FROM_DOUBLE, J_DOUBLE, J_INT);
	assert_conversion_mimic_stack(OPC_D2L, EXPR_CONVERSION_FROM_DOUBLE, J_DOUBLE, J_LONG);
	assert_conversion_mimic_stack(OPC_D2F, EXPR_CONVERSION_DOUBLE_TO_FLOAT, J_DOUBLE, J_FLOAT);
}

void test_convert_int_narrowing(void)
{
	assert_conversion_mimic_stack(OPC_I2B, EXPR_CONVERSION, J_INT, J_BYTE);
	assert_conversion_mimic_stack(OPC_I2C, EXPR_CONVERSION, J_INT, J_CHAR);
	assert_conversion_mimic_stack(OPC_I2S, EXPR_CONVERSION, J_INT, J_SHORT);
}
