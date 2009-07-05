#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "jit/exception.h"

#include "vm/class.h"
#include "vm/classloader.h"
#include "vm/die.h"
#include "vm/guard-page.h"
#include "vm/jni.h"
#include "vm/method.h"
#include "vm/object.h"
#include "vm/preload.h"

#define check_null(x)							\
	if (x == NULL) {						\
		signal_new_exception(vm_java_lang_NullPointerException, \
				     NULL);				\
		return NULL;						\
	}

jclass vm_jni_find_class(struct vm_jni_env *env, const char *name)
{
	struct vm_class *class;

	class = classloader_load(name);
	if (!class) {
		signal_new_exception(vm_java_lang_NoClassDefFoundError,
				     name);
		return NULL;
	}

	vm_class_ensure_init(class);
	if (exception_occurred())
		return NULL;

	return class->object;
}

jmethodID vm_jni_get_method_id(struct vm_jni_env *env, jclass clazz,
			       const char *name, const char *sig)
{
	struct vm_method *mb;
	struct vm_class *class;

	check_null(clazz);

	class = vm_class_get_class_from_class_object(clazz);
	check_null(class);

	vm_class_ensure_init(class);
	if (exception_occurred())
		return NULL;

	mb = vm_class_get_method(class, name, sig);
	if (!mb) {
		signal_new_exception(vm_java_lang_NoSuchMethodError, NULL);
		return NULL;
	}

	return mb;
}

jfieldID vm_jni_get_field_id(struct vm_jni_env *env, jclass clazz,
			     const char *name, const char *sig)
{
	struct vm_field *fb;
	struct vm_class *class;

	check_null(clazz);

	class = vm_class_get_class_from_class_object(clazz);
	check_null(class);

	vm_class_ensure_init(class);
	if (exception_occurred())
		return NULL;

	fb = vm_class_get_field(class, name, sig);
	if (!fb) {
		signal_new_exception(vm_java_lang_NoSuchFieldError, NULL);
		return NULL;
	}

	return fb;
}

/*
 * The JNI native interface table.
 * See: http://java.sun.com/j2se/1.4.2/docs/guide/jni/spec/functions.html
 */
void *vm_jni_native_interface[] = {
	/* 0 */
	NULL,
	NULL,
	NULL,
	NULL,
	NULL, /* GetVersion */

	/* 5 */
	NULL, /* DefineClass */
	vm_jni_find_class,
	NULL,
	NULL,
	NULL,

	/* 10 */
	NULL, /* GetSuperclass */
	NULL, /* IsAssignableFrom */
	NULL,
	NULL, /* Throw */
	NULL, /* ThrowNew */

	/* 15 */
	NULL, /* ExceptionOccurred */
	NULL, /* ExceptionDescribe */
	NULL, /* ExceptionClear */
	NULL, /* FatalError */
	NULL,

	/* 20 */
	NULL,
	NULL, /* NewGlobalRef */
	NULL, /* DeleteGlobalRef */
	NULL, /* DeleteLocalRef */
	NULL, /* IsSameObject */

	/* 25 */
	NULL,
	NULL,
	NULL, /* AllocObject */
	NULL, /* NewObject */
	NULL, /* NewObjectV */

	/* 30 */
	NULL, /* NewObjectA */
	NULL, /* GetObjectClass */
	NULL, /* IsInstanceOf */
	vm_jni_get_method_id,
	NULL, /* CallObjectMethod */

	/* 35 */
	NULL, /* CallObjectMethodV */
	NULL, /* CallObjectMethodA */
	NULL, /* CallBooleanMethod */
	NULL, /* CallBooleanMethodV */
	NULL, /* CallBooleanMethodA */

	/* 40 */
	NULL, /* CallByteMethod */
	NULL, /* CallByteMethodV */
	NULL, /* CallByteMethodA */
	NULL, /* CallCharMethod */
	NULL, /* CallCharMethodV */

	/* 45 */
	NULL, /* CallCharMethodA */
	NULL, /* CallShortMethod */
	NULL, /* CallShortMethodV */
	NULL, /* CallShortMethodA */
	NULL, /* CallIntMethod */

	/* 50 */
	NULL, /* CallIntMethodV */
	NULL, /* CallIntMethodA */
	NULL, /* CallLongMethod */
	NULL, /* CallLongMethodV */
	NULL, /* CallLongMethodA */

	/* 55 */
	NULL, /* CallFloatMethod */
	NULL, /* CallFloatMethodV */
	NULL, /* CallFloatMethodA */
	NULL, /* CallDoubleMethod */
	NULL, /* CallDoubleMethodV */

	/* 60 */
	NULL, /* CallDoubleMethodA */
	NULL, /* CallVoidMethod */
	NULL, /* CallVoidMethodV */
	NULL, /* CallVoidMethodA */
	NULL, /* CallNonvirtualObjectMethod */

	/* 65 */
	NULL, /* CallNonvirtualObjectMethodV */
	NULL, /* CallNonvirtualObjectMethodA */
	NULL, /* CallNonvirtualBooleanMethod */
	NULL, /* CallNonvirtualBooleanMethodV */
	NULL, /* CallNonvirtualBooleanMethodA */

	/* 70 */
	NULL, /* CallNonvirtualByteMethod */
	NULL, /* CallNonvirtualByteMethodV */
	NULL, /* CallNonvirtualByteMethodA */
	NULL, /* CallNonvirtualCharMethod */
	NULL, /* CallNonvirtualCharMethodV */

	/* 75 */
	NULL, /* CallNonvirtualCharMethodA */
	NULL, /* CallNonvirtualShortMethod */
	NULL, /* CallNonvirtualShortMethodV */
	NULL, /* CallNonvirtualShortMethodA */
	NULL, /* CallNonvirtualIntMethod */

	/* 80 */
	NULL, /* CallNonvirtualIntMethodV */
	NULL, /* CallNonvirtualIntMethodA */
	NULL, /* CallNonvirtualLongMethod */
	NULL, /* CallNonvirtualLongMethodV */
	NULL, /* CallNonvirtualLongMethodA */

	/* 85 */
	NULL, /* CallNonvirtualFloatMethod */
	NULL, /* CallNonvirtualFloatMethodV */
	NULL, /* CallNonvirtualFloatMethodA */
	NULL, /* CallNonvirtualDoubleMethod */
	NULL, /* CallNonvirtualDoubleMethodV */

	/* 90 */
	NULL, /* CallNonvirtualDoubleMethodA */
	NULL, /* CallNonvirtualVoidMethod */
	NULL, /* CallNonvirtualVoidMethodV */
	NULL, /* CallNonvirtualVoidMethodA */
	vm_jni_get_field_id,

	/* 95 */
	NULL, /* GetObjectField */
	NULL, /* GetBooleanField */
	NULL, /* GetByteField */
	NULL, /* GetCharField */
	NULL, /* GetShortField */

	/* 100 */
	NULL, /* GetIntField */
	NULL, /* GetLongField */
	NULL, /* GetFloatField */
	NULL, /* GetDoubleField */
	NULL, /* SetObjectField */

	/* 105 */
	NULL, /* SetBooleanField */
	NULL, /* SetByteField */
	NULL, /* SetCharField */
	NULL, /* SetShortField */
	NULL, /* SetIntField */

	/* 110 */
	NULL, /* SetLongField */
	NULL, /* SetFloatField */
	NULL, /* SetDoubleField */
	NULL, /* GetStaticMethodID */
	NULL, /* CallStaticObjectMethod */

	/* 115 */
	NULL, /* CallStaticObjectMethodV */
	NULL, /* CallStaticObjectMethodA */
	NULL, /* CallStaticBooleanMethod */
	NULL, /* CallStaticBooleanMethodV */
	NULL, /* CallStaticBooleanMethodA */

	/* 120 */
	NULL, /* CallStaticByteMethod */
	NULL, /* CallStaticByteMethodV */
	NULL, /* CallStaticByteMethodA */
	NULL, /* CallStaticCharMethod */
	NULL, /* CallStaticCharMethodV */

	/* 125 */
	NULL, /* CallStaticCharMethodA */
	NULL, /* CallStaticShortMethod */
	NULL, /* CallStaticShortMethodV */
	NULL, /* CallStaticShortMethodA */
	NULL, /* CallStaticIntMethod */

	/* 130 */
	NULL, /* CallStaticIntMethodV */
	NULL, /* CallStaticIntMethodA */
	NULL, /* CallStaticLongMethod */
	NULL, /* CallStaticLongMethodV */
	NULL, /* CallStaticLongMethodA */

	/* 135 */
	NULL, /* CallStaticFloatMethod */
	NULL, /* CallStaticFloatMethodV */
	NULL, /* CallStaticFloatMethodA */
	NULL, /* CallStaticDoubleMethod */
	NULL, /* CallStaticDoubleMethodV */

	/* 140 */
	NULL, /* CallStaticDoubleMethodA */
	NULL, /* CallStaticVoidMethod */
	NULL, /* CallStaticVoidMethodV */
	NULL, /* CallStaticVoidMethodA */
	NULL, /* GetStaticFieldID */

	/* 145 */
	NULL, /* GetStaticObjectField */
	NULL, /* GetStaticBooleanField */
	NULL, /* GetStaticByteField */
	NULL, /* GetStaticCharField */
	NULL, /* GetStaticShortField */

	/* 150 */
	NULL, /* GetStaticIntField */
	NULL, /* GetStaticLongField */
	NULL, /* GetStaticFloatField */
	NULL, /* GetStaticDoubleField */
	NULL, /* SetStaticObjectField */

	/* 155 */
	NULL, /* SetStaticBooleanField */
	NULL, /* SetStaticByteField */
	NULL, /* SetStaticCharField */
	NULL, /* SetStaticShortField */
	NULL, /* SetStaticIntField */

	/* 160 */
	NULL, /* SetStaticLongField */
	NULL, /* SetStaticFloatField */
	NULL, /* SetStaticDoubleField */
	NULL, /* NewString */
	NULL, /* GetStringLength */

	/* 165 */
	NULL, /* GetStringChars */
	NULL, /* ReleaseStringChars */
	NULL, /* NewStringUTF */
	NULL, /* GetStringUTFLength */
	NULL, /* GetStringUTFChars */

	/* 170 */
	NULL, /* ReleaseStringUTFChars */
	NULL, /* GetArrayLength */
	NULL, /* NewObjectArray */
	NULL, /* GetObjectArrayElement */
	NULL, /* SetObjectArrayElement */

	/* 175 */
	NULL, /* NewBooleanArray */
	NULL, /* NewByteArray */
	NULL, /* NewCharArray */
	NULL, /* NewShortArray */
	NULL, /* NewIntArray */

	/* 180 */
	NULL, /* NewLongArray */
	NULL, /* NewFloatArray */
	NULL, /* NewDoubleArray */
	NULL, /* GetBooleanArrayElements */
	NULL, /* GetByteArrayElements */

	/* 185 */
	NULL, /* GetCharArrayElements */
	NULL, /* GetShortArrayElements */
	NULL, /* GetIntArrayElements */
	NULL, /* GetLongArrayElements */
	NULL, /* GetFloatArrayElements */

	/* 190 */
	NULL, /* GetDoubleArrayElements */
	NULL, /* ReleaseBooleanArrayElements */
	NULL, /* ReleaseByteArrayElements */
	NULL, /* ReleaseCharArrayElements */
	NULL, /* ReleaseShortArrayElements */

	/* 195 */
	NULL, /* ReleaseIntArrayElements */
	NULL, /* ReleaseLongArrayElements */
	NULL, /* ReleaseFloatArrayElements */
	NULL, /* ReleaseDoubleArrayElements */
	NULL, /* GetBooleanArrayRegion */

	/* 200 */
	NULL, /* GetByteArrayRegion */
	NULL, /* GetCharArrayRegion */
	NULL, /* GetShortArrayRegion */
	NULL, /* GetIntArrayRegion */
	NULL, /* GetLongArrayRegion */

	/* 205 */
	NULL, /* GetFloatArrayRegion */
	NULL, /* GetDoubleArrayRegion */
	NULL, /* SetBooleanArrayRegion */
	NULL, /* SetByteArrayRegion */
	NULL, /* SetCharArrayRegion */

	/* 210 */
	NULL, /* SetShortArrayRegion */
	NULL, /* SetIntArrayRegion */
	NULL, /* SetLongArrayRegion */
	NULL, /* SetFloatArrayRegion */
	NULL, /* SetDoubleArrayRegion */

	/* 215 */
	NULL, /* RegisterNatives */
	NULL, /* UnregisterNatives */
	NULL, /* MonitorEnter */
	NULL, /* MonitorExit */
	NULL, /* GetJavaVM */

	/* 220 */
	NULL, /* GetJavaVM */
};

struct vm_jni_env vm_jni_default_env = {
	.jni_table = vm_jni_native_interface,
};

struct vm_jni_env *vm_jni_get_jni_env()
{
	return &vm_jni_default_env;
}

static void *jni_not_implemented_trap;

void vm_jni_init(void)
{
	jni_not_implemented_trap = alloc_guard_page();
	if (!jni_not_implemented_trap)
		die("guard page alloc failed");

	/* Initialize traps in the vm_jni_default_native_interface[] */
	unsigned int table_size = ARRAY_SIZE(vm_jni_native_interface);
	for (unsigned int i = 0; i < table_size; i++) {
		if (vm_jni_native_interface[i])
			continue;

		vm_jni_native_interface[i] =
			jni_not_implemented_trap + i;
	}
}

bool vm_jni_check_trap(void *ptr)
{
	int table_size;
	int index;

	index = (int)(ptr - jni_not_implemented_trap);
	table_size = ARRAY_SIZE(vm_jni_native_interface);

	if (index < 0 || index >= table_size)
		return false;

	die("JNI handler for index %d not implemented.\n", index);
	return true;
}
