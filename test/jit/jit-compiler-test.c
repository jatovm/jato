/*
 * Copyright (C) 2006  Pekka Enberg
 */

#include <jit/compilation-unit.h>
#include <jit/jit-compiler.h>

#include <vm/natives.h>
#include <vm/system.h>
#include <vm/vm.h>

#include <libharness.h>

#include "vm-utils.h"

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
	function = buffer_ptr(cu->objcode_buf);

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
	assert_not_null(cu->objcode_buf);
	assert_true(cu->is_compiled);
	assert_ptr_equals(buffer_ptr(cu->objcode_buf), objcode);

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
	objcode = buffer_ptr(cu->objcode_buf);
	jit_magic_trampoline(cu);
	assert_ptr_equals(objcode, buffer_ptr(cu->objcode_buf));

	free_compilation_unit(cu);
}

void test_jit_method_trampoline_compiles_and_invokes_method(void)
{
	struct compilation_unit *cu;
	sum_fn function;
	struct methodblock method = {
		.jit_code = sum_bytecode,
		.code_size = ARRAY_SIZE(sum_bytecode),
		.args_count = 2,
	};

	cu = alloc_compilation_unit(&method);
	method.trampoline = build_jit_trampoline(cu);

	function = trampoline_ptr(&method);
	assert_int_equals(1, function(0, 1));
	assert_int_equals(3, function(1, 2));
	
	free_jit_trampoline(method.trampoline);
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
	is_zero_fn function;
	struct methodblock method = {
		.jit_code = is_zero_bytecode,
		.code_size = ARRAY_SIZE(is_zero_bytecode),
		.args_count = 1,
	};

	cu = alloc_compilation_unit(&method);
	method.trampoline = build_jit_trampoline(cu);

	function = trampoline_ptr(&method);
	assert_int_equals(0, function(1));
	assert_int_equals(1, function(0));
	
	free_jit_trampoline(method.trampoline);
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
	assert_ptr_equals(trampoline_ptr(&mb), actual);

	free_compilation_unit(mb.compilation_unit);
	free_jit_trampoline(mb.trampoline);
}

static int native_sum(int a, int b)
{
	return a + b;
}

static char native_sum_invoker[] = {
	OPC_ILOAD_0,
	OPC_ILOAD_1,
	OPC_INVOKESTATIC, 0x00, 0x00,
	OPC_IRETURN
};

void test_jitted_code_invokes_native_method(void)
{
	struct constant_pool *constant_pool;
	struct classblock *classblock;
	struct object *invoker_class;
	struct object *native_class;
	sum_fn function;
	u4 cp_infos[1];
	u1 cp_types[1];

	struct methodblock invoker_method = {
		.jit_code = native_sum_invoker,
		.code_size = ARRAY_SIZE(native_sum_invoker),
		.args_count = 2,
	};
	struct methodblock native_sum_method = {
		.args_count = 2,
		.access_flags = ACC_NATIVE,
		.type = "(I)II",
	};

	jit_prepare_for_exec(&invoker_method);
	jit_prepare_for_exec(&native_sum_method);

	invoker_class = new_class();

	classblock = CLASS_CB(invoker_class);
	classblock->constant_pool_count = 1;

	cp_infos[0] = (unsigned long) &native_sum_method;
	cp_types[0] = CONSTANT_Resolved;

	constant_pool = &classblock->constant_pool;
	constant_pool->info = cp_infos;
	constant_pool->type = cp_types;

	invoker_method.class = invoker_class;

	native_class = new_class();
	CLASS_CB(native_class)->name = "Natives";
	native_sum_method.name = "nativeSum";
	native_sum_method.class = native_class;

	vm_register_native("Natives", "nativeSum", native_sum);

	function = trampoline_ptr(&invoker_method);

	assert_int_equals(1, function(0, 1));
	assert_int_equals(3, function(1, 2));
	
	vm_unregister_natives();

	free_compilation_unit(invoker_method.compilation_unit);
	free_jit_trampoline(invoker_method.trampoline);
	free_compilation_unit(native_sum_method.compilation_unit);
	free_jit_trampoline(native_sum_method.trampoline);
	free(invoker_class);
	free(native_class);
}
