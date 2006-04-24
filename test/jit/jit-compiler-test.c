/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <compilation-unit.h>
#include <jit-compiler.h>

#include <vm/system.h>
#include <vm/vm.h>

#include <libharness.h>

typedef int (*sum_fn)(int, int);

static unsigned char sum_bytecode[] = { OPC_ILOAD_0, OPC_ILOAD_1, OPC_IADD, OPC_IRETURN };

void test_executing_jit_compiled_function(void)
{
	struct compilation_unit *cu;
	sum_fn function;
	struct methodblock method = {
		.code = sum_bytecode,
		.code_size = ARRAY_SIZE(sum_bytecode)
	};
	
	cu = alloc_compilation_unit(&method);
	jit_compile(cu);
	function = cu->objcode;

	assert_int_equals(1, function(0, 1));
	assert_int_equals(3, function(1, 2));
	
	free_compilation_unit(cu);
}

void test_magic_trampoline_compiles_uncompiled(void)
{
	void *objcode;
	struct compilation_unit *cu;
	struct methodblock method = {
		.code = sum_bytecode,
		.code_size = ARRAY_SIZE(sum_bytecode)
	};
	cu = alloc_compilation_unit(&method);

	assert_false(cu->is_compiled);
	objcode = jit_magic_trampoline(cu);
	assert_not_null(cu->objcode);
	assert_true(cu->is_compiled);
	assert_ptr_equals(cu->objcode, objcode);

	free_compilation_unit(cu);
}

void test_magic_trampoline_compiles_once(void)
{
	void *objcode;
	struct compilation_unit *cu;
	struct methodblock method = {
		.code = sum_bytecode,
		.code_size = ARRAY_SIZE(sum_bytecode)
	};

	cu = alloc_compilation_unit(&method);
	jit_magic_trampoline(cu);
	objcode = cu->objcode;
	jit_magic_trampoline(cu);
	assert_ptr_equals(objcode, cu->objcode);

	free_compilation_unit(cu);
}

void test_jit_method_trampoline_compiles_and_invokes_method(void)
{
	struct compilation_unit *cu;
	struct jit_trampoline *t;
	sum_fn function;
	struct methodblock method = {
		.code = sum_bytecode,
		.code_size = ARRAY_SIZE(sum_bytecode)
	};

	cu = alloc_compilation_unit(&method);
	t = build_jit_trampoline(cu);

	function = t->objcode;
	assert_int_equals(1, function(0, 1));
	assert_int_equals(3, function(1, 2));
	
	free_jit_trampoline(t);
	free_compilation_unit(cu);
}
