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
	/* test_nop(); */
	/* test_aconst_null(); */
	test_iconst_m1();
	test_iconst_0();
	test_iconst_1();
	test_iconst_2();
	test_iconst_3();
	test_iconst_4();
	test_iconst_5();
	/* test_lconst_0(); */
	/* test_lconst_1(); */
	/* test_fconst_0(); */
	/* test_fconst_1(); */
	/* test_fconst_2(); */
	/* test_dconst_0(); */
	/* test_dconst_1(); */
	/* test_bipush(); */
	/* test_sipush(); */
	/* test_ldc(); */
	/* test_ldc_w(); */
	/* test_ldc2_w(); */
	/* test_iload(); */
	/* test_lload(); */
	/* test_fload(); */
	/* test_dload(); */
	/* test_aload(); */
	/* test_iload_0(); */
	/* test_iload_1(); */
	/* test_iload_2(); */
	/* test_iload_3(); */
	/* test_lload_0(); */
	/* test_lload_1(); */
	/* test_lload_2(); */
	/* test_lload_3(); */
	/* test_fload_0(); */
	/* test_fload_1(); */
	/* test_fload_2(); */
	/* test_fload_3(); */
	/* test_dload_0(); */
	/* test_dload_1(); */
	/* test_dload_2(); */
	/* test_dload_3(); */
	/* test_aload_0(); */
	/* test_aload_1(); */
	/* test_aload_2(); */
	/* test_aload_3(); */
	/* test_iaload(); */
	/* test_laload(); */
	/* test_faload(); */
	/* test_daload(); */
	/* test_aaload(); */
	/* test_baload(); */
	/* test_caload(); */
	/* test_saload(); */
	/* test_istore(); */
	/* test_lstore(); */
	/* test_fstore(); */
	/* test_dstore(); */
	/* test_astore(); */
	/* test_istore_0(); */
	/* test_istore_1(); */
	/* test_istore_2(); */
	/* test_istore_3(); */
	/* test_lstore_0(); */
	/* test_lstore_1(); */
	/* test_lstore_2(); */
	/* test_lstore_3(); */
	/* test_fstore_0(); */
	/* test_fstore_1(); */
	/* test_fstore_2(); */
	/* test_fstore_3(); */
	/* test_dstore_0(); */
	/* test_dstore_1(); */
	/* test_dstore_2(); */
	/* test_dstore_3(); */
	/* test_astore_0(); */
	/* test_astore_1(); */
	/* test_astore_2(); */
	/* test_astore_3(); */
	/* test_iastore(); */
	/* test_lastore(); */
	/* test_fastore(); */
	/* test_dastore(); */
	/* test_aastore(); */
	/* test_bastore(); */
	/* test_castore(); */
	/* test_sastore(); */
	/* test_pop(); */
	/* test_pop2(); */
	/* test_dup(); */
	/* test_dup_x1(); */
	/* test_dup_x2(); */
	/* test_dup2(); */
	/* test_dup2_x1(); */
	/* test_dup2_x2(); */
	/* test_swap(); */
	/* test_iadd(); */
	/* test_ladd(); */
	/* test_fadd(); */
	/* test_dadd(); */
	/* test_isub(); */
	/* test_lsub(); */
	/* test_fsub(); */
	/* test_dsub(); */
	/* test_imul(); */
	/* test_lmul(); */
	/* test_fmul(); */
	/* test_dmul(); */
	/* test_idiv(); */
	/* test_ldiv(); */
	/* test_fdiv(); */
	/* test_ddiv(); */
	/* test_irem(); */
	/* test_lrem(); */
	/* test_frem(); */
	/* test_drem(); */
	/* test_ineg(); */
	/* test_lneg(); */
	/* test_fneg(); */
	/* test_dneg(); */
	/* test_ishl(); */
	/* test_lshl(); */
	/* test_ishr(); */
	/* test_lshr(); */
	/* test_iushr(); */
	/* test_lushr(); */
	/* test_iand(); */
	/* test_land(); */
	/* test_ior(); */
	/* test_lor(); */
	/* test_ixor(); */
	/* test_lxor(); */
	/* test_iinc(); */
	/* test_i2l(); */
	/* test_i2f(); */
	/* test_i2d(); */
	/* test_l2i(); */
	/* test_l2f(); */
	/* test_l2d(); */
	/* test_f2i(); */
	/* test_f2l(); */
	/* test_f2d(); */
	/* test_d2i(); */
	/* test_d2l(); */
	/* test_d2f(); */
	/* test_i2b(); */
	/* test_i2c(); */
	/* test_i2s(); */
	/* test_lcmp(); */
	/* test_fcmpl(); */
	/* test_fcmpg(); */
	/* test_dcmpl(); */
	/* test_dcmpg(); */
	/* test_ifeq(); */
	/* test_ifne(); */
	/* test_iflt(); */
	/* test_ifge(); */
	/* test_ifgt(); */
	/* test_ifle(); */
	/* test_if_icmpeq(); */
	/* test_if_icmpne(); */
	/* test_if_icmplt(); */
	/* test_if_icmpge(); */
	/* test_if_icmpgt(); */
	/* test_if_icmple(); */
	/* test_if_acmpeq(); */
	/* test_if_acmpne(); */
	/* test_goto(); */
	/* test_jsr(); */
	/* test_ret(); */
	/* test_tableswitch(); */
	/* test_lookupswitch(); */
	/* test_ireturn(); */
	/* test_lreturn(); */
	/* test_freturn(); */
	/* test_dreturn(); */
	/* test_areturn(); */
	/* test_return(); */
	/* test_getstatic(); */
	/* test_putstatic(); */
	/* test_getfield(); */
	/* test_putfield(); */
	/* test_invokevirtual(); */
	/* test_invokespecial(); */
	/* test_invokestatic(); */
	/* test_invokeinterface(); */
	/* test_new(); */
	/* test_newarray(); */
	/* test_anewarray(); */
	/* test_arraylength(); */
	/* test_athrow(); */
	/* test_checkcast(); */
	/* test_instanceof(); */
	/* test_monitorenter(); */
	/* test_monitorexit(); */
	/* test_wide(); */
	/* test_multianewarray(); */
	/* test_ifnull(); */
	/* test_ifnonnull(); */
	/* test_goto_w(); */
	/* test_jsr_w(); */
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
