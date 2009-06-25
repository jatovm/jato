/*
 * Copyright (C) 2005  Pekka Enberg
 */

#include <jit/compiler.h>
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
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};
	struct basic_block *bb;

	bb = __alloc_simple_bb(&method);
	convert_to_ir(bb->b_parent);
	assert_true(stack_is_empty(bb->mimic_stack));
	__free_simple_bb(bb);
}

/* MISSING: jsr */

/* MISSING: ret */

/* MISSING: tableswitch */

/* MISSING: lookupswitch */

/* MISSING: getfield */

/* MISSING: putfield */

/* MISSING: new */

/* MISSING: newarray */

/* MISSING: athrow */

/* MISSING: checkcast */

/* MISSING: instanceof */

/* MISSING: monitorenter */

/* MISSING: monitorexit */

/* MISSING: wide */

/* MISSING: ifnull */

/* MISSING: ifnonnull */

/* MISSING: goto_w */

/* MISSING: jsr_w */

void test_converts_complete_basic_block(void)
{
	unsigned char code[] = { OPC_ILOAD_0, OPC_ILOAD_1, OPC_IADD, OPC_IRETURN };
	struct vm_method method = {
		.code_attribute.code = code,
		.code_attribute.code_length = ARRAY_SIZE(code),
	};
	struct basic_block *bb;

	bb = __alloc_simple_bb(&method); 
	convert_to_ir(bb->b_parent);

	assert_false(list_is_empty(&bb->stmt_list));
	assert_true(stack_is_empty(bb->mimic_stack));

	__free_simple_bb(bb);	
}
