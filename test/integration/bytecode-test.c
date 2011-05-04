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

#include <inttypes.h>
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

static jdouble do_jdouble_execute(uint8_t *code, unsigned long code_length)
{
	struct cafebabe_method_info method_info;
	struct vm_object class_object;
	struct vm_method vmm;
	struct vm_class vmc;
	jdouble (*method)(void);

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

static jfloat do_jfloat_execute(uint8_t *code, unsigned long code_length)
{
	struct cafebabe_method_info method_info;
	struct vm_object class_object;
	struct vm_method vmm;
	struct vm_class vmc;
	jfloat (*method)(void);

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

static jobject do_jobject_execute(uint8_t *code, unsigned long code_length)
{
	struct cafebabe_method_info method_info;
	struct vm_object class_object;
	struct vm_method vmm;
	struct vm_class vmc;
	jobject (*method)(void);

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
#define jlong_run(bytecode) do_execute(bytecode, ARRAY_SIZE(bytecode))
#define jdouble_run(bytecode) do_jdouble_execute(bytecode, ARRAY_SIZE(bytecode))
#define jfloat_run(bytecode) do_jfloat_execute(bytecode, ARRAY_SIZE(bytecode))
#define jobject_run(bytecode) do_jobject_execute(bytecode, ARRAY_SIZE(bytecode))

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

static void do_assert_long_equals(const char *function, const char *file, int line, jlong expected, jlong actual)
{
	if (expected != actual)
		die("%s:%d::%s: Expected %" PRId64 ", but was: %" PRId64, file, line, function, expected, actual);

	nr_assertions++;
}

#define assert_long_equals(expected, actual) do_assert_long_equals(__func__, __FILE__, __LINE__, expected, actual)

static void do_assert_double_equals(const char *function, const char *file, int line, jdouble expected, jdouble actual)
{
	if (expected != actual)
		die("%s:%d::%s: Expected %f, but was: %f", file, line, function, expected, actual);

	nr_assertions++;
}

#define assert_double_equals(expected, actual) do_assert_double_equals(__func__, __FILE__, __LINE__, expected, actual)

static void do_assert_float_equals(const char *function, const char *file, int line, jfloat expected, jfloat actual)
{
	if (expected != actual)
		die("%s:%d::%s: Expected %f, but was: %f", file, line, function, expected, actual);

	nr_assertions++;
}

#define assert_float_equals(expected, actual) do_assert_float_equals(__func__, __FILE__, __LINE__, expected, actual)

static void do_assert_object_equals(const char *function, const char *file, int line, jobject expected, jobject actual)
{
	if (expected != actual)
		die("%s:%d::%s: Expected %d, but was: %d", file, line, function, expected, actual);

	nr_assertions++;
}

#define assert_object_equals(expected, actual) do_assert_object_equals(__func__, __FILE__, __LINE__, expected, actual)

static void test_bipush(void)
{
	uint8_t bytecode[] = { OPC_BIPUSH, 0x10, OPC_IRETURN };

	assert_int_equals(16, execute(bytecode));
}

static void test_sipush(void)
{
	uint8_t bytecode[] = { OPC_SIPUSH, 0x01, 0x00, OPC_IRETURN };

	assert_int_equals(256, execute(bytecode));
}

static void test_ifle(void)
{
	uint8_t bytecode_1[] = { OPC_ICONST_M1, OPC_IFLE, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };
	uint8_t bytecode_2[] = { OPC_ICONST_0 , OPC_IFLE, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, execute(bytecode_1));
	assert_int_equals(1, execute(bytecode_2));
}

static void test_ifgt(void)
{
	uint8_t bytecode_1[] = { OPC_ICONST_1, OPC_IFGT, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };
	uint8_t bytecode_2[] = { OPC_ICONST_0, OPC_IFGT, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, execute(bytecode_1));
	assert_int_equals(2, execute(bytecode_2));
}

static void test_ifge(void)
{
	uint8_t bytecode_1[] = { OPC_ICONST_1, OPC_IFGE, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };
	uint8_t bytecode_2[] = { OPC_ICONST_0, OPC_IFGE, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, execute(bytecode_1));
	assert_int_equals(1, execute(bytecode_2));
}

static void test_iflt(void)
{
	uint8_t bytecode_1[] = { OPC_ICONST_M1, OPC_IFLT, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };
	uint8_t bytecode_2[] = { OPC_ICONST_0 , OPC_IFLT, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, execute(bytecode_1));
	assert_int_equals(2, execute(bytecode_2));
}

static void test_ifne(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_IFNE, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, execute(bytecode));
}

static void test_lcmp(void)
{
	uint8_t bytecode_l[] = { OPC_LCONST_0, OPC_LCONST_1, OPC_LCMP, OPC_LRETURN };
	uint8_t bytecode_g[] = { OPC_LCONST_1, OPC_LCONST_0, OPC_LCMP, OPC_LRETURN };
	uint8_t bytecode_e[] = { OPC_LCONST_1, OPC_LCONST_1, OPC_LCMP, OPC_LRETURN };

	assert_long_equals(-1, execute(bytecode_l));
	assert_long_equals(1, execute(bytecode_g));
	assert_long_equals(0, execute(bytecode_e));
}

static void test_ifeq(void)
{
	uint8_t bytecode[] = { OPC_ICONST_0, OPC_IFEQ, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, execute(bytecode));
}

static void test_goto(void)
{
	uint8_t bytecode[] = { OPC_GOTO, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, execute(bytecode));
}

static void test_goto_w(void)
{
	uint8_t bytecode[] = { OPC_GOTO_W, 0x00, 0x00, 0x00, 0x07, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, execute(bytecode));
}

static void test_ifnull(void)
{
	uint8_t bytecode[] = { OPC_ACONST_NULL, OPC_IFNULL, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, execute(bytecode));
}

static void test_ifnonnull(void)
{
	uint8_t bytecode[] = { OPC_ACONST_NULL, OPC_IFNONNULL, 0x00, 0x00, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(2, execute(bytecode));
}

static void test_ireturn(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, execute(bytecode));
}

static void test_lreturn(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LRETURN };

	assert_long_equals(1, execute(bytecode));
}

static void test_freturn(void)
{
	uint8_t bytecode[] = { OPC_FCONST_1, OPC_FRETURN };

	assert_float_equals(1.0, jfloat_run(bytecode));
}

static void test_dreturn(void)
{
	uint8_t bytecode[] = { OPC_DCONST_1, OPC_DRETURN };

	assert_double_equals(1.0, jdouble_run(bytecode));
}

static void test_areturn(void)
{
	uint8_t bytecode[] = { OPC_ACONST_NULL, OPC_ARETURN };

	assert_object_equals(NULL, jobject_run(bytecode));
}

static void test_lxor(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LCONST_1, OPC_LXOR, OPC_LRETURN };

	assert_long_equals(0, execute(bytecode));
}

static void test_ixor(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_ICONST_1, OPC_IXOR, OPC_IRETURN };

	assert_int_equals(0, execute(bytecode));
}

static void test_lor(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LCONST_0, OPC_LOR, OPC_LRETURN };

	assert_long_equals(1, execute(bytecode));
}

static void test_ior(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_ICONST_0, OPC_IOR, OPC_IRETURN };

	assert_int_equals(1, execute(bytecode));
}

static void test_land(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LCONST_0, OPC_LAND, OPC_LRETURN };

	assert_long_equals(0, execute(bytecode));
}

static void test_iand(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_ICONST_0, OPC_IAND, OPC_IRETURN };

	assert_int_equals(0, execute(bytecode));
}

static void test_lushr(void)
{
	uint8_t bytecode[] = { OPC_LCONST_0, OPC_LCONST_1, OPC_LUSHR, OPC_LRETURN };

	assert_long_equals(0, execute(bytecode));
}

static void test_iushr(void)
{
	uint8_t bytecode[] = { OPC_ICONST_0, OPC_ICONST_1, OPC_IUSHR, OPC_IRETURN };

	assert_int_equals(0, execute(bytecode));
}

static void test_lshr(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LCONST_1, OPC_LSHR, OPC_LRETURN };

	assert_long_equals(0, execute(bytecode));
}

static void test_ishr(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_ICONST_1, OPC_ISHR, OPC_IRETURN };

	assert_int_equals(0, execute(bytecode));
}

static void test_lshl(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LCONST_1, OPC_LSHL, OPC_LRETURN };

	assert_long_equals(2, execute(bytecode));
}

static void test_ishl(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_ICONST_1, OPC_ISHL, OPC_IRETURN };

	assert_int_equals(2, execute(bytecode));
}

static void test_lneg(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LNEG, OPC_LRETURN };

	assert_long_equals(-1, execute(bytecode));
}

static void test_ineg(void)
{
	uint8_t bytecode[] = { OPC_ICONST_2, OPC_INEG, OPC_IRETURN };

	assert_int_equals(-2, execute(bytecode));
}

static void test_dneg(void)
{
	uint8_t bytecode[] = { OPC_DCONST_1, OPC_DNEG, OPC_DRETURN };

	assert_double_equals(-1.0, jdouble_run(bytecode));
}

static void test_fneg(void)
{
	uint8_t bytecode[] = { OPC_FCONST_2, OPC_FNEG, OPC_FRETURN };

	assert_int_equals(-2.0, jfloat_run(bytecode));
}

static void test_lrem(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LCONST_1, OPC_LREM, OPC_LRETURN };

	assert_long_equals(0, execute(bytecode));
}

static void test_irem(void)
{
	uint8_t bytecode[] = { OPC_ICONST_4, OPC_ICONST_3, OPC_IREM, OPC_IRETURN };

	assert_int_equals(1, execute(bytecode));
}

static void test_drem(void)
{
	uint8_t bytecode[] = { OPC_DCONST_1, OPC_DCONST_1, OPC_DREM, OPC_DRETURN };

	assert_long_equals(0.0, jdouble_run(bytecode));
}

static void test_frem(void)
{
	uint8_t bytecode[] = { OPC_FCONST_2, OPC_FCONST_1, OPC_FREM, OPC_FRETURN };

	assert_float_equals(0.0, jfloat_run(bytecode));
}

static void test_ldiv(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LCONST_1, OPC_LDIV, OPC_LRETURN };

	assert_long_equals(1, execute(bytecode));
}

static void test_idiv(void)
{
	uint8_t bytecode[] = { OPC_ICONST_4, OPC_ICONST_2, OPC_IDIV, OPC_IRETURN };

	assert_int_equals(2, execute(bytecode));
}

static void test_lmul(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LCONST_1, OPC_IMUL, OPC_LRETURN };

	assert_long_equals(1, execute(bytecode));
}

static void test_imul(void)
{
	uint8_t bytecode[] = { OPC_ICONST_2, OPC_ICONST_3, OPC_IMUL, OPC_IRETURN };

	assert_int_equals(6, execute(bytecode));
}

static void test_lsub(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LCONST_1, OPC_LSUB, OPC_LRETURN };

	assert_long_equals(0, execute(bytecode));
}

static void test_isub(void)
{
	uint8_t bytecode[] = { OPC_ICONST_3, OPC_ICONST_2, OPC_ISUB, OPC_IRETURN };

	assert_int_equals(1, execute(bytecode));
}

static void test_dadd(void)
{
	uint8_t bytecode[] = { OPC_DCONST_1, OPC_DCONST_1, OPC_DADD, OPC_DRETURN };

	assert_double_equals(2.0, jdouble_run(bytecode));
}

static void test_dsub(void)
{
	uint8_t bytecode[] = { OPC_DCONST_1, OPC_DCONST_1, OPC_DSUB, OPC_DRETURN };

	assert_double_equals(0.0, jdouble_run(bytecode));
}

static void test_dmul(void)
{
	uint8_t bytecode[] = { OPC_DCONST_1, OPC_DCONST_1, OPC_DMUL, OPC_DRETURN };

	assert_double_equals(1.0, jdouble_run(bytecode));
}

static void test_ddiv(void)
{
	uint8_t bytecode[] = { OPC_DCONST_1, OPC_DCONST_1, OPC_DDIV, OPC_DRETURN };

	assert_double_equals(1.0, jdouble_run(bytecode));
}

static void test_fadd(void)
{
	uint8_t bytecode[] = { OPC_FCONST_1, OPC_FCONST_2, OPC_FADD, OPC_FRETURN };

	assert_float_equals(3.0, jfloat_run(bytecode));
}

static void test_fsub(void)
{
	uint8_t bytecode[] = { OPC_FCONST_2, OPC_FCONST_1, OPC_FSUB, OPC_FRETURN };

	assert_float_equals(1.0, jfloat_run(bytecode));
}

static void test_fmul(void)
{
	uint8_t bytecode[] = { OPC_FCONST_2, OPC_FCONST_2, OPC_FMUL, OPC_FRETURN };

	assert_float_equals(4.0, jfloat_run(bytecode));
}

static void test_fdiv(void)
{
	uint8_t bytecode[] = { OPC_FCONST_2, OPC_FCONST_1, OPC_FDIV, OPC_FRETURN };

	assert_float_equals(2.0, jfloat_run(bytecode));
}

static void test_ladd(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LCONST_1, OPC_LADD, OPC_LRETURN };

	assert_long_equals(2, execute(bytecode));
}

static void test_iadd(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_ICONST_2, OPC_IADD, OPC_IRETURN };

	assert_int_equals(3, execute(bytecode));
}

static void test_pop2(void)
{
	uint8_t bytecode[] = { OPC_ICONST_3, OPC_ICONST_2, OPC_ICONST_1, OPC_POP2, OPC_IRETURN };

	assert_int_equals(3, execute(bytecode));
}

static void test_pop(void)
{
	uint8_t bytecode[] = { OPC_ICONST_2, OPC_ICONST_1, OPC_POP, OPC_IRETURN };

	assert_int_equals(2, execute(bytecode));
}

static void test_dconst_1(void)
{
	uint8_t bytecode[] = { OPC_DCONST_1, OPC_DRETURN };

	assert_double_equals(1.0, jdouble_run(bytecode));
}

static void test_dconst_0(void)
{
	uint8_t bytecode[] = { OPC_DCONST_0, OPC_DRETURN };

	assert_double_equals(0.0, jdouble_run(bytecode));
}

static void test_fconst_2(void)
{
	uint8_t bytecode[] = { OPC_FCONST_2, OPC_FRETURN };

	assert_float_equals(2.0, jfloat_run(bytecode));
}

static void test_fconst_1(void)
{
	uint8_t bytecode[] = { OPC_FCONST_1, OPC_FRETURN };

	assert_float_equals(1.0, jfloat_run(bytecode));
}

static void test_fconst_0(void)
{
	uint8_t bytecode[] = { OPC_FCONST_0, OPC_FRETURN };

	assert_float_equals(0.0, jfloat_run(bytecode));
}

static void test_lconst_1(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LRETURN };

	assert_long_equals(1, jlong_run(bytecode));
}

static void test_lconst_0(void)
{
	uint8_t bytecode[] = { OPC_LCONST_0, OPC_LRETURN };

	assert_long_equals(0, jlong_run(bytecode));
}

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

static void test_aconst_null(void)
{
	uint8_t bytecode[] = { OPC_ACONST_NULL, OPC_IRETURN };

	assert_object_equals(NULL, jobject_run(bytecode));
}

static void test_iconst_m1(void)
{
	uint8_t bytecode[] = { OPC_ICONST_M1, OPC_IRETURN };

	assert_int_equals(-1, execute(bytecode));
}

static void run_tests(void)
{
	/* test_nop(); */
	test_aconst_null();
	test_iconst_m1();
	test_iconst_0();
	test_iconst_1();
	test_iconst_2();
	test_iconst_3();
	test_iconst_4();
	test_iconst_5();
	test_lconst_0();
	test_lconst_1();
	test_fconst_0();
	test_fconst_1();
	test_fconst_2();
	test_dconst_0();
	test_dconst_1();
	test_bipush();
	test_sipush();
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
	test_pop();
	test_pop2();
	/* test_dup(); */
	/* test_dup_x1(); */
	/* test_dup_x2(); */
	/* test_dup2(); */
	/* test_dup2_x1(); */
	/* test_dup2_x2(); */
	/* test_swap(); */
	test_iadd();
	test_ladd();
	test_fadd();
	test_dadd();
	test_isub();
	test_lsub();
	test_fsub();
	test_dsub();
	test_imul();
	test_lmul();
	test_fmul();
	test_dmul();
	test_idiv();
	test_ldiv();
	test_fdiv();
	test_ddiv();
	test_irem();
	test_lrem();
	test_frem();
	test_drem();
	test_ineg();
	test_lneg();
	test_fneg();
	test_dneg();
	test_ishl();
	test_lshl();
	test_ishr();
	test_lshr();
	test_iushr();
	test_lushr();
	test_iand();
	test_land();
	test_ior();
	test_lor();
	test_ixor();
	test_lxor();
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
	test_lcmp();
	/* test_fcmpl(); */
	/* test_fcmpg(); */
	/* test_dcmpl(); */
	/* test_dcmpg(); */
	test_ifeq();
	test_ifne();
	test_iflt();
	test_ifge();
	test_ifgt();
	test_ifle();
	/* test_if_icmpeq(); */
	/* test_if_icmpne(); */
	/* test_if_icmplt(); */
	/* test_if_icmpge(); */
	/* test_if_icmpgt(); */
	/* test_if_icmple(); */
	/* test_if_acmpeq(); */
	/* test_if_acmpne(); */
	test_goto();
	/* test_jsr(); */
	/* test_ret(); */
	/* test_tableswitch(); */
	/* test_lookupswitch(); */
	test_ireturn();
	test_lreturn();
	test_freturn();
	test_dreturn();
	test_areturn();
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
	test_ifnull();
	test_ifnonnull();
	test_goto_w();
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
