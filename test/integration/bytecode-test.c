#include "jit/compilation-unit.h"
#include "jit/cu-mapping.h"
#include "jit/exception.h"
#include "jit/compiler.h"
#include "jit/text.h"
#include "jit/gdb.h"

#include "cafebabe/code_attribute.h"
#include "cafebabe/method_info.h"

#include "valgrind/valgrind.h"

#include "vm/classloader.h"
#include "vm/reference.h"
#include "vm/opcodes.h"
#include "vm/preload.h"
#include "vm/method.h"
#include "vm/signal.h"
#include "vm/string.h"
#include "vm/class.h"
#include "vm/die.h"

#include "arch/init.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

bool running_on_valgrind;
bool opt_ssa_enable;

static jint do_execute(uint8_t *code, unsigned long code_length)
{
	struct cafebabe_method_info method_info;
	struct vm_object class_object;
	struct vm_method vmm;
	struct vm_class vmc;
	jint (*method)(void);

	class_object	= (struct vm_object) {
	};

	vmc		= (struct vm_class) {
		.object		= &class_object,
		.name		= "Foo",
		.state		= VM_CLASS_LINKED,
	};

	vm_class_init(&vmc);

	method_info	= (struct cafebabe_method_info) {
		.access_flags	= CAFEBABE_METHOD_ACC_STATIC,
	};

	vmm		= (struct vm_method) {
		.method		= &method_info,
		.name		= "foo",
		.class		= &vmc,
		.code_attribute = (struct cafebabe_code_attribute) {
			.code		= code,
			.code_length	= code_length,
		},
	};

	if (vm_method_prepare_jit(&vmm))
		die("unable to prepare method");

	method = vm_method_trampoline_ptr(&vmm);

	return method();
}

#define execute(bytecode) do_execute(bytecode, ARRAY_SIZE(bytecode))

static void init(void)
{
	if (RUNNING_ON_VALGRIND) {
		printf("JIT: Enabling workarounds for valgrind.\n");
		running_on_valgrind = true;
	}

	arch_init();
	init_literals_hash_map();

	gc_init();
	init_exec_env();
	vm_reference_init();

	classloader_init();

	init_vm_objects();

	jit_text_init();

	gdb_init();

	setup_signal_handlers();
	init_cu_mapping();
	init_exceptions();

	static_fixup_init();
	vm_jni_init();
}

static unsigned long nr_assertions;

static void do_assert_int_equals(const char *function, const char *file, int line, jint expected, jint actual)
{
	if (expected != actual)
		die("%s:%d::%s: Expected %d, but was: %d", file, line, function, expected, actual);

	nr_assertions++;
}

#define assert_int_equals(expected, actual) do_assert_int_equals(__func__, __FILE__, __LINE__, expected, actual)

static void test_iconst_5(void)
{
	uint8_t bytecode[] = { OPC_ICONST_5, OPC_IRETURN };

	assert_int_equals(5, execute(bytecode));
}

static void test_iconst_4(void)
{
	uint8_t bytecode[] = { OPC_ICONST_4, OPC_IRETURN };

	assert_int_equals(4, execute(bytecode));
}

static void test_iconst_3(void)
{
	uint8_t bytecode[] = { OPC_ICONST_3, OPC_IRETURN };

	assert_int_equals(3, execute(bytecode));
}

static void test_iconst_2(void)
{
	uint8_t bytecode[] = { OPC_ICONST_2, OPC_IRETURN };

	assert_int_equals(2, execute(bytecode));
}

static void test_iconst_1(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, execute(bytecode));
}

static void test_iconst_0(void)
{
	uint8_t bytecode[] = { OPC_ICONST_0, OPC_IRETURN };

	assert_int_equals(0, execute(bytecode));
}

static void test_iconst_m1(void)
{
	uint8_t bytecode[] = { OPC_ICONST_M1, OPC_IRETURN };

	assert_int_equals(-1, execute(bytecode));
}

static void run_tests(void)
{
	test_iconst_m1();
	test_iconst_0();
	test_iconst_1();
	test_iconst_2();
	test_iconst_3();
	test_iconst_4();
	test_iconst_5();
}

int main(int argc, char *argv[])
{
	dont_gc = true;

	opt_trace_machine_code = true;

	preload_finished = true;

	init();

	run_tests();

	printf("%ld assertion(s). Tests OK\n", nr_assertions);

	return EXIT_SUCCESS;
}
