/*
 * Copyright (C) 2005  Pekka Enberg
 */

#include <jit/compiler.h>
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

	cu = alloc_simple_compilation_unit(&method);

	convert_to_ir(cu);
	assert_true(stack_is_empty(cu->expr_stack));

	free_compilation_unit(cu);
}

/* MISSING: jsr */

/* MISSING: ret */

/* MISSING: tableswitch */

/* MISSING: lookupswitch */

/* MISSING: getfield */

/* MISSING: putfield */

/* MISSING: new */

/* MISSING: newarray */

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
