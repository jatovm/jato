/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <libharness.h>
#include <system.h>
#include <jit-compiler.h>
#include <compilation-unit.h>
#include <jam.h>

typedef int (*sum_fn)(int, int);

unsigned char sum_bytecode[] = { OPC_ILOAD_0, OPC_ILOAD_1, OPC_IADD, OPC_IRETURN };

void test_executing_jit_compiled_function(void)
{
	struct compilation_unit *cu;
	sum_fn function;
	
	cu = alloc_compilation_unit(sum_bytecode, ARRAY_SIZE(sum_bytecode));
	jit_compile(cu);
	function = cu->objcode;

	assert_int_equals(1, function(0, 1));
	assert_int_equals(3, function(1, 2));
	
	free_compilation_unit(cu);
}

void test_magic_trampoline_compiles_uncompiled(void)
{
	struct compilation_unit *cu;
	cu = alloc_compilation_unit(sum_bytecode, ARRAY_SIZE(sum_bytecode));

	assert_false(cu->is_compiled);
	jit_magic_trampoline(cu);
	assert_not_null(cu->objcode);
	assert_true(cu->is_compiled);
	
	free_compilation_unit(cu);
}

void test_magic_trampoline_compiles_once(void)
{
	void *objcode;
	struct compilation_unit *cu;

	cu = alloc_compilation_unit(sum_bytecode, ARRAY_SIZE(sum_bytecode));
	jit_magic_trampoline(cu);
	objcode = cu->objcode;
	jit_magic_trampoline(cu);
	assert_ptr_equals(objcode, cu->objcode);

	free_compilation_unit(cu);
}
