/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <jit/compilation-unit.h>
#include <jit/jit-compiler.h>

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
		.jit_code = sum_bytecode,
		.code_size = ARRAY_SIZE(sum_bytecode),
		.args_count = 2,
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
		.jit_code = sum_bytecode,
		.code_size = ARRAY_SIZE(sum_bytecode),
		.args_count = 2,
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
		.jit_code = sum_bytecode,
		.code_size = ARRAY_SIZE(sum_bytecode),
		.args_count = 2,
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
		.jit_code = sum_bytecode,
		.code_size = ARRAY_SIZE(sum_bytecode),
		.args_count = 2,
	};

	cu = alloc_compilation_unit(&method);
	t = build_jit_trampoline(cu);

	function = t->objcode;
	assert_int_equals(1, function(0, 1));
	assert_int_equals(3, function(1, 2));
	
	free_jit_trampoline(t);
	free_compilation_unit(cu);
}

typedef int (*is_zero_fn)(int);

static unsigned char is_zero_bytecode[] = {
	OPC_ILOAD_0,
	OPC_ICONST_0,
	OPC_IF_ICMPEQ, 0x00, 0x05,

	OPC_ICONST_0,
	OPC_IRETURN,

	OPC_ICONST_1,
	OPC_IRETURN,
};

void test_magic_trampoline_compiles_all_basic_blocks(void)
{
	struct compilation_unit *cu;
	struct jit_trampoline *t;
	is_zero_fn function;
	struct methodblock method = {
		.jit_code = is_zero_bytecode,
		.code_size = ARRAY_SIZE(is_zero_bytecode),
		.args_count = 1,
	};

	cu = alloc_compilation_unit(&method);
	t = build_jit_trampoline(cu);

	function = t->objcode;
	assert_int_equals(0, function(1));
	assert_int_equals(1, function(0));
	
	free_jit_trampoline(t);
	free_compilation_unit(cu);
}

static char java_main[] = { OPC_RETURN };

void test_jit_prepare_for_exec_returns_trampoline_objcode(void)
{
	struct methodblock mb;
	mb.jit_code = java_main;
	mb.code_size = ARRAY_SIZE(java_main);
	mb.compilation_unit = NULL;
	void *actual = jit_prepare_for_exec(&mb);
	assert_not_null(mb.compilation_unit);
	assert_not_null(mb.trampoline);
	assert_ptr_equals(mb.trampoline->objcode, actual);

	free_compilation_unit(mb.compilation_unit);
	free_jit_trampoline(mb.trampoline);
}
