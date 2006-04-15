/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <libharness.h>
#include <jit/jit.h>
#include <vm/vm.h>
#include <system.h>
#include <stddef.h>
#include <compilation-unit.h>
#include <jit-compiler.h>

static char java_main[] = { OPC_RETURN };

void test_jit_prepare_for_exec_returns_trampoline_objcode(void)
{
	struct methodblock mb;
	mb.code = java_main;
	mb.code_size = ARRAY_SIZE(java_main);
	mb.compilation_unit = NULL;
	void *actual = jit_prepare_for_exec(&mb);
	assert_not_null(mb.compilation_unit);
	assert_not_null(mb.trampoline);
	assert_ptr_equals(mb.trampoline->objcode, actual);
}
