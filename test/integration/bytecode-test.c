#include "jit/compilation-unit.h"
#include "jit/cu-mapping.h"
#include "jit/exception.h"
#include "jit/llvm/core.h"
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
#include <math.h>		/* isnan et al */

bool running_on_valgrind;
bool opt_ssa_enable;
bool opt_llvm_enable;
bool opt_llvm_verbose;

static jint do_jint_execute(uint8_t *code, unsigned long code_length)
{
	struct cafebabe_constant_pool constant_pool[2];
	struct cafebabe_method_info method_info;
	struct cafebabe_class class_info;
	struct vm_object class_object;
	struct vm_method vmm;
	struct vm_class vmc;
	jint (*method)(void);

	constant_pool[1].tag		= CAFEBABE_CONSTANT_TAG_INTEGER;
	constant_pool[1].integer_.bytes	= 0xffffffffUL;

	class_info	= (struct cafebabe_class) {
		.constant_pool		= constant_pool,
		.constant_pool_count	= ARRAY_SIZE(constant_pool),
	};

	class_object	= (struct vm_object) {
	};

	vmc		= (struct vm_class) {
		.object		= &class_object,
		.name		= "Foo",
		.state		= VM_CLASS_LINKED,
		.class		= &class_info,
	};

	vm_class_init(&vmc);

	method_info	= (struct cafebabe_method_info) {
		.access_flags	= CAFEBABE_METHOD_ACC_STATIC,
	};

	vmm		= (struct vm_method) {
		.method		= &method_info,
		.name		= "foo",
		.type		= "()I",
		.class		= &vmc,
		.code_attribute = (struct cafebabe_code_attribute) {
			.max_locals	= 8,
			.code		= code,
			.code_length	= code_length,
		},
	};

	parse_method_type(&vmm);

	if (vm_method_prepare_jit(&vmm))
		die("unable to prepare method");

	method = vm_method_trampoline_ptr(&vmm);

	return method();
}

static jlong do_jlong_execute(uint8_t *code, unsigned long code_length)
{
	struct cafebabe_constant_pool constant_pool[2];
	struct cafebabe_method_info method_info;
	struct cafebabe_class class_info;
	struct vm_object class_object;
	struct vm_method vmm;
	struct vm_class vmc;
	jlong (*method)(void);

	constant_pool[1].tag		= CAFEBABE_CONSTANT_TAG_LONG;
	constant_pool[1].long_.low_bytes	= 0xffffffffUL;
	constant_pool[1].long_.high_bytes	= 0xffffffffUL;

	class_info	= (struct cafebabe_class) {
		.constant_pool		= constant_pool,
		.constant_pool_count	= ARRAY_SIZE(constant_pool),
	};

	class_object	= (struct vm_object) {
	};

	vmc		= (struct vm_class) {
		.object		= &class_object,
		.name		= "Foo",
		.state		= VM_CLASS_LINKED,
		.class		= &class_info,
	};

	vm_class_init(&vmc);

	method_info	= (struct cafebabe_method_info) {
		.access_flags	= CAFEBABE_METHOD_ACC_STATIC,
	};

	vmm		= (struct vm_method) {
		.method		= &method_info,
		.name		= "foo",
		.type		= "()J",
		.class		= &vmc,
		.code_attribute = (struct cafebabe_code_attribute) {
			.max_locals	= 8,
			.code		= code,
			.code_length	= code_length,
		},
	};

	parse_method_type(&vmm);

	if (vm_method_prepare_jit(&vmm))
		die("unable to prepare method");

	method = vm_method_trampoline_ptr(&vmm);

	return method();
}

enum {
	CP_IDX_DOUBLE_NAN	= 1,
	CP_IDX_DOUBLE_ZERO	= 2,
	CP_IDX_DOUBLE_POINT_75	= 3,

	CP_DOUBLE_SIZE		/* keep last */
};

static jdouble do_jdouble_execute(uint8_t *code, unsigned long code_length)
{
	struct cafebabe_constant_pool constant_pool[CP_DOUBLE_SIZE];
	struct cafebabe_method_info method_info;
	struct cafebabe_class class_info;
	struct vm_object class_object;
	struct vm_method vmm;
	struct vm_class vmc;
	jdouble (*method)(void);

	constant_pool[CP_IDX_DOUBLE_NAN].tag				= CAFEBABE_CONSTANT_TAG_DOUBLE;
	constant_pool[CP_IDX_DOUBLE_NAN].double_.low_bytes		= 0xffffffffUL;
	constant_pool[CP_IDX_DOUBLE_NAN].double_.high_bytes		= 0xffffffffUL;

	constant_pool[CP_IDX_DOUBLE_ZERO].tag				= CAFEBABE_CONSTANT_TAG_DOUBLE;
	constant_pool[CP_IDX_DOUBLE_ZERO].double_.low_bytes		= 0;
	constant_pool[CP_IDX_DOUBLE_ZERO].double_.high_bytes		= 0;

	constant_pool[CP_IDX_DOUBLE_POINT_75].tag			= CAFEBABE_CONSTANT_TAG_DOUBLE;
	constant_pool[CP_IDX_DOUBLE_POINT_75].double_.low_bytes		= 0;
	constant_pool[CP_IDX_DOUBLE_POINT_75].double_.high_bytes	= 0x3fe80000UL;

	class_info	= (struct cafebabe_class) {
		.constant_pool		= constant_pool,
		.constant_pool_count	= ARRAY_SIZE(constant_pool),
	};

	class_object	= (struct vm_object) {
	};

	vmc		= (struct vm_class) {
		.object		= &class_object,
		.name		= "Foo",
		.state		= VM_CLASS_LINKED,
		.class		= &class_info,
	};

	vm_class_init(&vmc);

	method_info	= (struct cafebabe_method_info) {
		.access_flags	= CAFEBABE_METHOD_ACC_STATIC,
	};

	vmm		= (struct vm_method) {
		.method		= &method_info,
		.name		= "foo",
		.type		= "()D",
		.class		= &vmc,
		.code_attribute = (struct cafebabe_code_attribute) {
			.max_locals	= 4,
			.code		= code,
			.code_length	= code_length,
		},
	};

	parse_method_type(&vmm);

	if (vm_method_prepare_jit(&vmm))
		die("unable to prepare method");

	method = vm_method_trampoline_ptr(&vmm);

	return method();
}

enum {
	CP_IDX_FLOAT_ZERO	= 1,
	CP_IDX_FLOAT_NAN	= 2,
	CP_IDX_FLOAT_POINT_75	= 3,

	CP_FLOAT_SIZE		/* keep last */
};

static jfloat do_jfloat_execute(uint8_t *code, unsigned long code_length)
{
	struct cafebabe_constant_pool constant_pool[CP_FLOAT_SIZE];
	struct cafebabe_method_info method_info;
	struct cafebabe_class class_info;
	struct vm_object class_object;
	struct vm_method vmm;
	struct vm_class vmc;
	jfloat (*method)(void);

	constant_pool[CP_IDX_FLOAT_ZERO].tag			= CAFEBABE_CONSTANT_TAG_FLOAT;
	constant_pool[CP_IDX_FLOAT_ZERO].float_.bytes		= 0;

	constant_pool[CP_IDX_FLOAT_NAN].tag			= CAFEBABE_CONSTANT_TAG_FLOAT;
	constant_pool[CP_IDX_FLOAT_NAN].float_.bytes	=	 0xffffffffUL;

	constant_pool[CP_IDX_FLOAT_POINT_75].tag		= CAFEBABE_CONSTANT_TAG_FLOAT;
	constant_pool[CP_IDX_FLOAT_POINT_75].float_.bytes	= 0x3f400000UL;

	class_info	= (struct cafebabe_class) {
		.constant_pool		= constant_pool,
		.constant_pool_count	= ARRAY_SIZE(constant_pool),
	};

	class_object	= (struct vm_object) {
	};

	vmc		= (struct vm_class) {
		.object		= &class_object,
		.name		= "Foo",
		.state		= VM_CLASS_LINKED,
		.class		= &class_info,
	};

	vm_class_init(&vmc);

	method_info	= (struct cafebabe_method_info) {
		.access_flags	= CAFEBABE_METHOD_ACC_STATIC,
	};

	vmm		= (struct vm_method) {
		.method		= &method_info,
		.name		= "foo",
		.type		= "()F",
		.class		= &vmc,
		.code_attribute = (struct cafebabe_code_attribute) {
			.max_locals	= 4,
			.code		= code,
			.code_length	= code_length,
		},
	};

	parse_method_type(&vmm);

	if (vm_method_prepare_jit(&vmm))
		die("unable to prepare method");

	method = vm_method_trampoline_ptr(&vmm);

	return method();
}

static jobject do_jobject_execute(uint8_t *code, unsigned long code_length)
{
	struct cafebabe_constant_pool constant_pool[2];
	struct cafebabe_method_info method_info;
	struct cafebabe_class class_info;
	struct vm_object class_object;
	struct vm_method vmm;
	struct vm_class vmc;
	jobject (*method)(void);

	class_info	= (struct cafebabe_class) {
		.constant_pool		= constant_pool,
		.constant_pool_count	= ARRAY_SIZE(constant_pool),
	};

	class_object	= (struct vm_object) {
	};

	vmc		= (struct vm_class) {
		.object		= &class_object,
		.name		= "Foo",
		.state		= VM_CLASS_LINKED,
		.class		= &class_info,
	};

	vm_class_init(&vmc);

	method_info	= (struct cafebabe_method_info) {
		.access_flags	= CAFEBABE_METHOD_ACC_STATIC,
	};

	vmm		= (struct vm_method) {
		.method		= &method_info,
		.name		= "foo",
		.type		= "()Ljava/lang/Object;",
		.class		= &vmc,
		.code_attribute = (struct cafebabe_code_attribute) {
			.max_locals	= 4,
			.code		= code,
			.code_length	= code_length,
		},
	};

	parse_method_type(&vmm);

	if (vm_method_prepare_jit(&vmm))
		die("unable to prepare method");

	method = vm_method_trampoline_ptr(&vmm);

	return method();
}

static void do_jvoid_execute(uint8_t *code, unsigned long code_length)
{
	struct cafebabe_constant_pool constant_pool[2];
	struct cafebabe_method_info method_info;
	struct cafebabe_class class_info;
	struct vm_object class_object;
	struct vm_method vmm;
	struct vm_class vmc;
	jobject (*method)(void);

	class_info	= (struct cafebabe_class) {
		.constant_pool		= constant_pool,
		.constant_pool_count	= ARRAY_SIZE(constant_pool),
	};

	class_object	= (struct vm_object) {
	};

	vmc		= (struct vm_class) {
		.object		= &class_object,
		.name		= "Foo",
		.state		= VM_CLASS_LINKED,
		.class		= &class_info,
	};

	vm_class_init(&vmc);

	method_info	= (struct cafebabe_method_info) {
		.access_flags	= CAFEBABE_METHOD_ACC_STATIC,
	};

	vmm		= (struct vm_method) {
		.method		= &method_info,
		.name		= "foo",
		.type		= "()V",
		.class		= &vmc,
		.code_attribute = (struct cafebabe_code_attribute) {
			.max_locals	= 4,
			.code		= code,
			.code_length	= code_length,
		},
	};

	parse_method_type(&vmm);

	if (vm_method_prepare_jit(&vmm))
		die("unable to prepare method");

	method = vm_method_trampoline_ptr(&vmm);

	method();
}

#define jint_run(bytecode) do_jint_execute(bytecode, ARRAY_SIZE(bytecode))
#define jlong_run(bytecode) do_jlong_execute(bytecode, ARRAY_SIZE(bytecode))
#define jdouble_run(bytecode) do_jdouble_execute(bytecode, ARRAY_SIZE(bytecode))
#define jfloat_run(bytecode) do_jfloat_execute(bytecode, ARRAY_SIZE(bytecode))
#define jobject_run(bytecode) do_jobject_execute(bytecode, ARRAY_SIZE(bytecode))
#define jvoid_run(bytecode) do_jvoid_execute(bytecode, ARRAY_SIZE(bytecode))

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

static void do_assert_true(const char *function, const char *file, int line, bool condition)
{
	if (!condition)
		die("%s:%d::%s: Expected true, but was false", file, line, function);

	nr_assertions++;
}

#define assert_true(condition) do_assert_true(__func__, __FILE__, __LINE__, condition)

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
		die("%s:%d::%s: Expected %p, but was: %p", file, line, function, expected, actual);

	nr_assertions++;
}

#define assert_object_equals(expected, actual) do_assert_object_equals(__func__, __FILE__, __LINE__, expected, actual)

static void test_bipush(void)
{
	uint8_t bytecode[] = { OPC_BIPUSH, 0x10, OPC_IRETURN };

	assert_int_equals(16, jint_run(bytecode));
}

#ifndef CONFIG_ARM
static void test_sipush(void)
{
	uint8_t bytecode[] = { OPC_SIPUSH, 0x01, 0x00, OPC_IRETURN };

	assert_int_equals(256, jint_run(bytecode));
}

static void test_ldc_double(void)
{
	uint8_t nan[]  = { OPC_LDC, CP_IDX_DOUBLE_NAN, 0x00, OPC_DRETURN };
	uint8_t zero[] = { OPC_LDC, CP_IDX_DOUBLE_ZERO, 0x00, OPC_DRETURN };
	uint8_t _75[]  = { OPC_LDC, CP_IDX_DOUBLE_POINT_75, 0x00, OPC_DRETURN };

	assert_true(isnan(jdouble_run(nan)));

	assert_double_equals(0.0, jdouble_run(zero));

	assert_double_equals(0.75, jdouble_run(_75));
}

static void test_ldc_float(void)
{
	uint8_t nan[]  = { OPC_LDC, CP_IDX_FLOAT_NAN, 0x00, OPC_FRETURN };
	uint8_t zero[] = { OPC_LDC, CP_IDX_FLOAT_ZERO, 0x00, OPC_FRETURN };
	uint8_t _75[]  = { OPC_LDC, CP_IDX_FLOAT_POINT_75, 0x00, OPC_FRETURN };

	assert_true(isnan(jfloat_run(nan)));

	assert_float_equals(0.0f, jfloat_run(zero));

	assert_float_equals(0.75f, jfloat_run(_75));
}

static void test_ldc_long(void)
{
	uint8_t bytecode[] = { OPC_LDC, 0x01, 0x00, OPC_LRETURN };

	assert_long_equals(-1, jlong_run(bytecode));
}

static void test_ldc_int(void)
{
	uint8_t bytecode[] = { OPC_LDC, 0x01, 0x00, OPC_IRETURN };

	assert_int_equals(-1, jint_run(bytecode));
}

static void test_ldc(void)
{
	test_ldc_int();
	test_ldc_long();
	test_ldc_float();
	test_ldc_double();
}

static void test_if_icmpeq(void)
{
	uint8_t bytecode_1[] = { OPC_ICONST_1, OPC_ICONST_1, OPC_IF_ICMPEQ, 0x00, 0x05, OPC_ICONST_1, OPC_IRETURN, OPC_ICONST_0, OPC_IRETURN };
	uint8_t bytecode_2[] = { OPC_ICONST_1, OPC_ICONST_2, OPC_IF_ICMPEQ, 0x00, 0x05, OPC_ICONST_1, OPC_IRETURN, OPC_ICONST_0, OPC_IRETURN };

	assert_int_equals(0, jint_run(bytecode_1));
	assert_int_equals(1, jint_run(bytecode_2));
}

static void test_if_icmpne(void)
{
	uint8_t bytecode_1[] = { OPC_ICONST_1, OPC_ICONST_1, OPC_IF_ICMPNE, 0x00, 0x05, OPC_ICONST_1, OPC_IRETURN, OPC_ICONST_0, OPC_IRETURN };
	uint8_t bytecode_2[] = { OPC_ICONST_1, OPC_ICONST_2, OPC_IF_ICMPNE, 0x00, 0x05, OPC_ICONST_1, OPC_IRETURN, OPC_ICONST_0, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode_1));
	assert_int_equals(0, jint_run(bytecode_2));
}

static void test_if_icmplt(void)
{
	uint8_t bytecode_1[] = { OPC_ICONST_1, OPC_ICONST_0, OPC_IF_ICMPLT, 0x00, 0x05, OPC_ICONST_1, OPC_IRETURN, OPC_ICONST_0, OPC_IRETURN };
	uint8_t bytecode_2[] = { OPC_ICONST_1, OPC_ICONST_2, OPC_IF_ICMPLT, 0x00, 0x05, OPC_ICONST_1, OPC_IRETURN, OPC_ICONST_0, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode_1));
	assert_int_equals(0, jint_run(bytecode_2));
}

static void test_if_icmpgt(void)
{
	uint8_t bytecode_1[] = { OPC_ICONST_1, OPC_ICONST_0, OPC_IF_ICMPGT, 0x00, 0x05, OPC_ICONST_1, OPC_IRETURN, OPC_ICONST_0, OPC_IRETURN };
	uint8_t bytecode_2[] = { OPC_ICONST_1, OPC_ICONST_2, OPC_IF_ICMPGT, 0x00, 0x05, OPC_ICONST_1, OPC_IRETURN, OPC_ICONST_0, OPC_IRETURN };

	assert_int_equals(0, jint_run(bytecode_1));
	assert_int_equals(1, jint_run(bytecode_2));
}

static void test_if_icmple(void)
{
	uint8_t bytecode_1[] = { OPC_ICONST_1, OPC_ICONST_0, OPC_IF_ICMPLE, 0x00, 0x05, OPC_ICONST_1, OPC_IRETURN, OPC_ICONST_0, OPC_IRETURN };
	uint8_t bytecode_2[] = { OPC_ICONST_1, OPC_ICONST_1, OPC_IF_ICMPLE, 0x00, 0x05, OPC_ICONST_1, OPC_IRETURN, OPC_ICONST_0, OPC_IRETURN };
	uint8_t bytecode_3[] = { OPC_ICONST_1, OPC_ICONST_2, OPC_IF_ICMPLE, 0x00, 0x05, OPC_ICONST_1, OPC_IRETURN, OPC_ICONST_0, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode_1));
	assert_int_equals(0, jint_run(bytecode_2));
	assert_int_equals(0, jint_run(bytecode_3));
}

static void test_if_icmpge(void)
{
	uint8_t bytecode_1[] = { OPC_ICONST_1, OPC_ICONST_0, OPC_IF_ICMPGT, 0x00, 0x05, OPC_ICONST_1, OPC_IRETURN, OPC_ICONST_0, OPC_IRETURN };
	uint8_t bytecode_2[] = { OPC_ICONST_1, OPC_ICONST_2, OPC_IF_ICMPGT, 0x00, 0x05, OPC_ICONST_1, OPC_IRETURN, OPC_ICONST_0, OPC_IRETURN };
	uint8_t bytecode_3[] = { OPC_ICONST_1, OPC_ICONST_1, OPC_IF_ICMPGT, 0x00, 0x05, OPC_ICONST_1, OPC_IRETURN, OPC_ICONST_0, OPC_IRETURN };

	assert_int_equals(0, jint_run(bytecode_1));
	assert_int_equals(1, jint_run(bytecode_2));
	assert_int_equals(1, jint_run(bytecode_3));
}

static void test_if_acmpeq(void)
{
	uint8_t bytecode_1[] = { OPC_ACONST_NULL, OPC_ACONST_NULL, OPC_IF_ACMPEQ, 0x00, 0x05, OPC_ICONST_1, OPC_IRETURN, OPC_ICONST_0, OPC_IRETURN };

	assert_int_equals(0, jint_run(bytecode_1));
}

static void test_if_acmpne(void)
{
	uint8_t bytecode_1[] = { OPC_ACONST_NULL, OPC_ACONST_NULL, OPC_IF_ACMPNE, 0x00, 0x05, OPC_ICONST_1, OPC_IRETURN, OPC_ICONST_0, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode_1));
}

static void test_ifle(void)
{
	uint8_t bytecode_1[] = { OPC_ICONST_M1, OPC_IFLE, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };
	uint8_t bytecode_2[] = { OPC_ICONST_0 , OPC_IFLE, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode_1));
	assert_int_equals(1, jint_run(bytecode_2));
}

static void test_ifgt(void)
{
	uint8_t bytecode_1[] = { OPC_ICONST_1, OPC_IFGT, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };
	uint8_t bytecode_2[] = { OPC_ICONST_0, OPC_IFGT, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode_1));
	assert_int_equals(2, jint_run(bytecode_2));
}

static void test_ifge(void)
{
	uint8_t bytecode_1[] = { OPC_ICONST_1, OPC_IFGE, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };
	uint8_t bytecode_2[] = { OPC_ICONST_0, OPC_IFGE, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode_1));
	assert_int_equals(1, jint_run(bytecode_2));
}

static void test_iflt(void)
{
	uint8_t bytecode_1[] = { OPC_ICONST_M1, OPC_IFLT, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };
	uint8_t bytecode_2[] = { OPC_ICONST_0 , OPC_IFLT, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode_1));
	assert_int_equals(2, jint_run(bytecode_2));
}

static void test_ifne(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_IFNE, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode));
}

static void test_lcmp(void)
{
	uint8_t bytecode_l[] = { OPC_LCONST_0, OPC_LCONST_1, OPC_LCMP, OPC_IRETURN };
	uint8_t bytecode_g[] = { OPC_LCONST_1, OPC_LCONST_0, OPC_LCMP, OPC_IRETURN };
	uint8_t bytecode_e[] = { OPC_LCONST_1, OPC_LCONST_1, OPC_LCMP, OPC_IRETURN };

	assert_int_equals(-1, jint_run(bytecode_l));
	assert_int_equals(1, jint_run(bytecode_g));
	assert_int_equals(0, jint_run(bytecode_e));
}

static void test_fcmpl(void)
{
	uint8_t bytecode_l[] = { OPC_FCONST_0, OPC_FCONST_1, OPC_FCMPL, OPC_IRETURN };
	uint8_t bytecode_g[] = { OPC_FCONST_1, OPC_FCONST_0, OPC_FCMPL, OPC_IRETURN };
	uint8_t bytecode_e[] = { OPC_FCONST_2, OPC_FCONST_2, OPC_FCMPL, OPC_IRETURN };

	assert_int_equals(-1, jint_run(bytecode_l));
	assert_int_equals(1, jint_run(bytecode_g));
	assert_int_equals(0, jint_run(bytecode_e));
}

static void test_fcmpg(void)
{
	uint8_t bytecode_l[] = { OPC_FCONST_0, OPC_FCONST_1, OPC_FCMPG, OPC_IRETURN };
	uint8_t bytecode_g[] = { OPC_FCONST_1, OPC_FCONST_0, OPC_FCMPG, OPC_IRETURN };
	uint8_t bytecode_e[] = { OPC_FCONST_2, OPC_FCONST_2, OPC_FCMPG, OPC_IRETURN };

	assert_int_equals(-1, jint_run(bytecode_l));
	assert_int_equals(1, jint_run(bytecode_g));
	assert_int_equals(0, jint_run(bytecode_e));
}

static void test_dcmpl(void)
{
	uint8_t bytecode_l[] = { OPC_DCONST_0, OPC_DCONST_1, OPC_DCMPL, OPC_IRETURN };
	uint8_t bytecode_g[] = { OPC_DCONST_1, OPC_DCONST_0, OPC_DCMPL, OPC_IRETURN };
	uint8_t bytecode_e[] = { OPC_DCONST_1, OPC_DCONST_1, OPC_DCMPL, OPC_IRETURN };

	assert_int_equals(-1, jint_run(bytecode_l));
	assert_int_equals(1, jint_run(bytecode_g));
	assert_int_equals(0, jint_run(bytecode_e));
}

static void test_dcmpg(void)
{
	uint8_t bytecode_l[] = { OPC_DCONST_0, OPC_DCONST_1, OPC_DCMPG, OPC_IRETURN };
	uint8_t bytecode_g[] = { OPC_DCONST_1, OPC_DCONST_0, OPC_DCMPG, OPC_IRETURN };
	uint8_t bytecode_e[] = { OPC_DCONST_1, OPC_DCONST_1, OPC_DCMPG, OPC_IRETURN };

	assert_int_equals(-1, jint_run(bytecode_l));
	assert_int_equals(1, jint_run(bytecode_g));
	assert_int_equals(0, jint_run(bytecode_e));
}

static void test_ifeq(void)
{
	uint8_t bytecode[] = { OPC_ICONST_0, OPC_IFEQ, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode));
}

static void test_goto(void)
{
	uint8_t bytecode[] = { OPC_GOTO, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode));
}

static void test_goto_w(void)
{
	uint8_t bytecode[] = { OPC_GOTO_W, 0x00, 0x00, 0x00, 0x07, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode));
}

static void test_ifnull(void)
{
	uint8_t bytecode[] = { OPC_ACONST_NULL, OPC_IFNULL, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode));
}

static void test_ifnonnull(void)
{
	uint8_t bytecode[] = { OPC_ACONST_NULL, OPC_IFNONNULL, 0x00, 0x05, OPC_ICONST_2, OPC_IRETURN, OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(2, jint_run(bytecode));
}
#endif

static void test_ireturn(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode));
}

static void test_lreturn(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LRETURN };

	assert_long_equals(1, jlong_run(bytecode));
}

#ifndef CONFIG_ARM
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
#endif

static void test_return(void)
{
	uint8_t bytecode[] = { OPC_RETURN };

	/* OPC_RETURN doesn't have a return value */
	jvoid_run(bytecode);
}

#ifndef CONFIG_ARM
static void test_lxor(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LCONST_1, OPC_LXOR, OPC_LRETURN };

	assert_long_equals(0, jlong_run(bytecode));
}

static void test_ixor(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_ICONST_1, OPC_IXOR, OPC_IRETURN };

	assert_int_equals(0, jint_run(bytecode));
}

static void test_lor(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LCONST_0, OPC_LOR, OPC_LRETURN };

	assert_long_equals(1, jlong_run(bytecode));
}

static void test_ior(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_ICONST_0, OPC_IOR, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode));
}

static void test_land(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LCONST_0, OPC_LAND, OPC_LRETURN };

	assert_long_equals(0, jlong_run(bytecode));
}

static void test_iand(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_ICONST_0, OPC_IAND, OPC_IRETURN };

	assert_int_equals(0, jint_run(bytecode));
}

static void test_lushr(void)
{
	uint8_t bytecode[] = { OPC_LCONST_0, OPC_LCONST_1, OPC_LUSHR, OPC_LRETURN };

	assert_long_equals(0, jlong_run(bytecode));
}

static void test_iushr(void)
{
	uint8_t bytecode[] = { OPC_ICONST_0, OPC_ICONST_1, OPC_IUSHR, OPC_IRETURN };

	assert_int_equals(0, jint_run(bytecode));
}

static void test_lshr(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LCONST_1, OPC_LSHR, OPC_LRETURN };

	assert_long_equals(0, jlong_run(bytecode));
}

static void test_ishr(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_ICONST_1, OPC_ISHR, OPC_IRETURN };

	assert_int_equals(0, jint_run(bytecode));
}

static void test_lshl(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LCONST_1, OPC_LSHL, OPC_LRETURN };

	assert_long_equals(2, jlong_run(bytecode));
}

static void test_ishl(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_ICONST_1, OPC_ISHL, OPC_IRETURN };

	assert_int_equals(2, jint_run(bytecode));
}

static void test_lneg(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LNEG, OPC_LRETURN };

	assert_long_equals(-1, jlong_run(bytecode));
}

static void test_ineg(void)
{
	uint8_t bytecode[] = { OPC_ICONST_2, OPC_INEG, OPC_IRETURN };

	assert_int_equals(-2, jint_run(bytecode));
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

	assert_long_equals(0, jlong_run(bytecode));
}

static void test_irem(void)
{
	uint8_t bytecode[] = { OPC_ICONST_4, OPC_ICONST_3, OPC_IREM, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode));
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

	assert_long_equals(1, jlong_run(bytecode));
}

static void test_idiv(void)
{
	uint8_t bytecode[] = { OPC_ICONST_4, OPC_ICONST_2, OPC_IDIV, OPC_IRETURN };

	assert_int_equals(2, jint_run(bytecode));
}

static void test_lmul(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LCONST_1, OPC_IMUL, OPC_LRETURN };

	assert_long_equals(1, jlong_run(bytecode));
}

static void test_imul(void)
{
	uint8_t bytecode[] = { OPC_ICONST_2, OPC_ICONST_3, OPC_IMUL, OPC_IRETURN };

	assert_int_equals(6, jint_run(bytecode));
}
#endif

static void test_lsub(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LCONST_1, OPC_LSUB, OPC_LRETURN };

	assert_long_equals(0, jlong_run(bytecode));
}

static void test_isub(void)
{
	uint8_t bytecode[] = { OPC_ICONST_3, OPC_ICONST_2, OPC_ISUB, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode));
}

#ifndef CONFIG_ARM
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
#endif

static void test_ladd(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LCONST_1, OPC_LADD, OPC_LRETURN };

	assert_long_equals(2, jlong_run(bytecode));
}

static void test_iadd(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_ICONST_2, OPC_IADD, OPC_IRETURN };

	assert_int_equals(3, jint_run(bytecode));
}

#ifndef CONFIG_ARM
static void test_swap(void)
{
	uint8_t bytecode_1[] = { OPC_ICONST_1, OPC_ICONST_2, OPC_SWAP, OPC_IRETURN };
	uint8_t bytecode_2[] = { OPC_DCONST_1, OPC_DCONST_0, OPC_SWAP, OPC_DRETURN };
	uint8_t bytecode_3[] = { OPC_LCONST_1, OPC_LCONST_0, OPC_SWAP, OPC_LRETURN };
	uint8_t bytecode_4[] = { OPC_FCONST_1, OPC_FCONST_0, OPC_SWAP, OPC_FRETURN };

	assert_int_equals(1, jint_run(bytecode_1));
	assert_double_equals(1.0, jdouble_run(bytecode_2));
	assert_long_equals(1, jlong_run(bytecode_3));
	assert_float_equals(1.0, jfloat_run(bytecode_4));
}

static void test_pop2(void)
{
	uint8_t bytecode[] = { OPC_ICONST_3, OPC_ICONST_2, OPC_ICONST_1, OPC_POP2, OPC_IRETURN };

	assert_int_equals(3, jint_run(bytecode));
}

static void test_pop(void)
{
	uint8_t bytecode[] = { OPC_ICONST_2, OPC_ICONST_1, OPC_POP, OPC_IRETURN };

	assert_int_equals(2, jint_run(bytecode));
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
#endif

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

	assert_int_equals(5, jint_run(bytecode));
}

static void test_iconst_4(void)
{
	uint8_t bytecode[] = { OPC_ICONST_4, OPC_IRETURN };

	assert_int_equals(4, jint_run(bytecode));
}

static void test_iconst_3(void)
{
	uint8_t bytecode[] = { OPC_ICONST_3, OPC_IRETURN };

	assert_int_equals(3, jint_run(bytecode));
}

static void test_iconst_2(void)
{
	uint8_t bytecode[] = { OPC_ICONST_2, OPC_IRETURN };

	assert_int_equals(2, jint_run(bytecode));
}

static void test_iconst_1(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode));
}

static void test_iconst_0(void)
{
	uint8_t bytecode[] = { OPC_ICONST_0, OPC_IRETURN };

	assert_int_equals(0, jint_run(bytecode));
}

static void test_aconst_null(void)
{
	uint8_t bytecode[] = { OPC_ACONST_NULL, OPC_IRETURN };

	assert_object_equals(NULL, jobject_run(bytecode));
}

static void test_iconst_m1(void)
{
	uint8_t bytecode[] = { OPC_ICONST_M1, OPC_IRETURN };

	assert_int_equals(-1, jint_run(bytecode));
}

static void test_istore_0(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_ISTORE_0, OPC_ILOAD_0, OPC_IRETURN};

	assert_int_equals(1, jint_run(bytecode));
}

static void test_istore_1(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_ISTORE_1, OPC_ILOAD_1, OPC_IRETURN};

	assert_int_equals(1, jint_run(bytecode));
}

static void test_istore_2(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_ISTORE_2, OPC_ILOAD_2, OPC_IRETURN};

	assert_int_equals(1, jint_run(bytecode));
}

static void test_istore_3(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_ISTORE_3, OPC_ILOAD_3, OPC_IRETURN};

	assert_int_equals(1, jint_run(bytecode));
}

static void test_lstore_0(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LSTORE_0, OPC_LLOAD_0, OPC_LRETURN};

	assert_long_equals(1, jlong_run(bytecode));
}

static void test_lstore_1(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LSTORE_1, OPC_LLOAD_1, OPC_LRETURN};

	assert_long_equals(1, jlong_run(bytecode));
}

static void test_lstore_2(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LSTORE_2, OPC_LLOAD_2, OPC_LRETURN};

	assert_long_equals(1, jlong_run(bytecode));
}

static void test_lstore_3(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_LSTORE_3, OPC_LLOAD_3, OPC_LRETURN};

	assert_long_equals(1, jlong_run(bytecode));
}

#ifndef CONFIG_ARM
static void test_fstore_0(void)
{
	uint8_t bytecode[] = { OPC_FCONST_1, OPC_FSTORE_0, OPC_FLOAD_0, OPC_FRETURN};

	assert_float_equals(1, jfloat_run(bytecode));
}

static void test_fstore_1(void)
{
	uint8_t bytecode[] = { OPC_FCONST_1, OPC_FSTORE_1, OPC_FLOAD_1, OPC_FRETURN};

	assert_float_equals(1, jfloat_run(bytecode));
}

static void test_fstore_2(void)
{
	uint8_t bytecode[] = { OPC_FCONST_1, OPC_FSTORE_2, OPC_FLOAD_2, OPC_FRETURN};

	assert_float_equals(1, jfloat_run(bytecode));
}

static void test_fstore_3(void)
{
	uint8_t bytecode[] = { OPC_FCONST_1, OPC_FSTORE_3, OPC_FLOAD_3, OPC_FRETURN};

	assert_float_equals(1, jfloat_run(bytecode));
}

static void test_dstore_0(void)
{
	uint8_t bytecode[] = { OPC_DCONST_1, OPC_DSTORE_0, OPC_DLOAD_0, OPC_DRETURN};

	assert_double_equals(1, jdouble_run(bytecode));
}

static void test_dstore_1(void)
{
	uint8_t bytecode[] = { OPC_DCONST_1, OPC_DSTORE_1, OPC_DLOAD_1, OPC_DRETURN};

	assert_double_equals(1, jdouble_run(bytecode));
}

static void test_dstore_2(void)
{
	uint8_t bytecode[] = { OPC_DCONST_1, OPC_DSTORE_2, OPC_DLOAD_2, OPC_DRETURN};

	assert_double_equals(1, jdouble_run(bytecode));
}

static void test_dstore_3(void)
{
	uint8_t bytecode[] = { OPC_DCONST_1, OPC_DSTORE_3, OPC_DLOAD_3, OPC_DRETURN};

	assert_double_equals(1, jdouble_run(bytecode));
}

static void test_astore_0(void)
{
	uint8_t bytecode[] = { OPC_ACONST_NULL, OPC_ASTORE_0, OPC_ALOAD_0, OPC_ARETURN};

	assert_object_equals(NULL, jobject_run(bytecode));
}

static void test_astore_1(void)
{
	uint8_t bytecode[] = { OPC_ACONST_NULL, OPC_ASTORE_1, OPC_ALOAD_1, OPC_ARETURN};

	assert_object_equals(NULL, jobject_run(bytecode));
}

static void test_astore_2(void)
{
	uint8_t bytecode[] = { OPC_ACONST_NULL, OPC_ASTORE_2, OPC_ALOAD_2, OPC_ARETURN};

	assert_object_equals(NULL, jobject_run(bytecode));
}

static void test_astore_3(void)
{
	uint8_t bytecode[] = { OPC_ACONST_NULL, OPC_ASTORE_3, OPC_ALOAD_3, OPC_ARETURN};

	assert_object_equals(NULL, jobject_run(bytecode));
}

static void test_i2f(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_I2F, OPC_FRETURN };

	assert_float_equals(1.0, jfloat_run(bytecode));
}

static void test_i2d(void)
{
	uint8_t bytecode[] = { OPC_ICONST_3, OPC_I2D, OPC_DRETURN };

	assert_double_equals(3.0 ,jdouble_run(bytecode));
}

static void test_l2i(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_L2I, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode));
}

static void test_l2f(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_L2F, OPC_FRETURN };

	assert_float_equals(1, jfloat_run(bytecode));
}

static void test_l2d(void)
{
	uint8_t bytecode[] = { OPC_LCONST_1, OPC_L2D, OPC_DRETURN };

	assert_double_equals(1.0, jdouble_run(bytecode));
}

static void test_f2i(void)
{
	uint8_t bytecode[] = { OPC_FCONST_1, OPC_F2I, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode));
}

static void test_f2l(void)
{
	uint8_t bytecode[] = { OPC_FCONST_1, OPC_F2L, OPC_LRETURN };

	assert_long_equals(1, jlong_run(bytecode));
}

static void test_f2d(void)
{
	uint8_t bytecode[] = { OPC_FCONST_2, OPC_F2D, OPC_DRETURN };

	assert_double_equals(2.0, jdouble_run(bytecode));
}

static void test_d2i(void)
{
	uint8_t bytecode[] = { OPC_DCONST_1, OPC_D2I, OPC_IRETURN };

	assert_int_equals(1, jint_run(bytecode));
}

static void test_d2l(void)
{
	uint8_t bytecode[] = { OPC_DCONST_1, OPC_D2L, OPC_LRETURN };

	assert_long_equals(1, jlong_run(bytecode));
}

static void test_d2f(void)
{
	uint8_t bytecode[] = { OPC_DCONST_1, OPC_D2F, OPC_FRETURN };

	assert_float_equals(1, jfloat_run(bytecode));
}

static void test_i2b(void)
{
	uint8_t bytecode[] = { OPC_BIPUSH, 0xFF, OPC_I2B, OPC_IRETURN };

	assert_int_equals(-1, jint_run(bytecode));
}

static void test_i2c(void)
{
	uint8_t bytecode[] = { OPC_SIPUSH, 0xFF, 0xFF, OPC_I2C, OPC_IRETURN };

	assert_int_equals(0xffff, jint_run(bytecode));
}

static void test_i2s(void)
{
	uint8_t bytecode[] = { OPC_SIPUSH, 0xFF, 0xFF, OPC_I2S, OPC_IRETURN };

	assert_int_equals(-1, jint_run(bytecode));
}

static void test_i2l(void)
{
	uint8_t bytecode[] = { OPC_ICONST_1, OPC_I2L, OPC_LRETURN };

	assert_long_equals(1, jlong_run(bytecode));
}
#endif

static void test_nop(void)
{
	uint8_t bytecode[] = { OPC_NOP, OPC_RETURN };

	jvoid_run(bytecode);
}

static void run_tests(void)
{
	test_nop();
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
#ifndef CONFIG_ARM
	test_fconst_0();
	test_fconst_1();
	test_fconst_2();
	test_dconst_0();
	test_dconst_1();
#endif
	test_bipush();
#ifndef CONFIG_ARM
	test_sipush();
	test_ldc();
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
#endif
	test_istore_0();
	test_istore_1();
	test_istore_2();
	test_istore_3();
	test_lstore_0();
	test_lstore_1();
	test_lstore_2();
	test_lstore_3();
#ifndef CONFIG_ARM
	test_fstore_0();
	test_fstore_1();
	test_fstore_2();
	test_fstore_3();
	test_dstore_0();
	test_dstore_1();
	test_dstore_2();
	test_dstore_3();
	test_astore_0();
	test_astore_1();
	test_astore_2();
	test_astore_3();
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
	test_swap();
#endif
	test_iadd();
	test_ladd();
#ifndef CONFIG_ARM
	test_fadd();
	test_dadd();
#endif
	test_isub();
	test_lsub();
#ifndef CONFIG_ARM
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
	test_i2l();
	test_i2f();
	test_i2d();
	test_l2i();
	test_l2f();
	test_l2d();
	test_f2i();
	test_f2l();
	test_f2d();
	test_d2i();
	test_d2l();
	test_d2f();
	test_i2b();
	test_i2c();
	test_i2s();
	test_lcmp();
	test_fcmpl();
	test_fcmpg();
	test_dcmpl();
	test_dcmpg();
	test_ifeq();
	test_ifne();
	test_iflt();
	test_ifge();
	test_ifgt();
	test_ifle();
	test_if_icmpeq();
	test_if_icmpne();
	test_if_icmplt();
	test_if_icmpge();
	test_if_icmpgt();
	test_if_icmple();
	test_if_acmpeq();
	test_if_acmpne();
	test_goto();
	/* test_jsr(); */
	/* test_ret(); */
	/* test_tableswitch(); */
	/* test_lookupswitch(); */
#endif
	test_ireturn();
	test_lreturn();
#ifndef CONFIG_ARM
	test_freturn();
	test_dreturn();
	test_areturn();
#endif
	test_return();
#ifndef CONFIG_ARM
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
#endif
}

int main(int argc, char *argv[])
{
	if (argc > 1) {
		if (!strcmp(argv[1], "-Xllvm")) {
			opt_llvm_enable		= true;

		} else if (!strcmp(argv[1], "-Xllvm:verbose")) {
			opt_llvm_enable		= true;
			opt_llvm_verbose	= true;
		}
	}

	if (opt_llvm_enable)
		llvm_init();

	dont_gc = true;

	preload_finished = true;

	init();

	run_tests();

	printf("%ld assertion(s). Tests OK\n", nr_assertions);

	return EXIT_SUCCESS;
}
