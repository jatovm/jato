/*
 * Copyright (c) 2009 Tomasz Grabiec
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

#include "jit/exception.h"

#include "vm/call.h"
#include "vm/class.h"
#include "vm/classloader.h"
#include "vm/die.h"
#include "vm/guard-page.h"
#include "vm/jni.h"
#include "vm/method.h"
#include "vm/object.h"
#include "vm/preload.h"
#include "vm/stack-trace.h"

#define check_null(x)							\
	if ((x) == NULL) {						\
		signal_new_exception(vm_java_lang_NullPointerException, \
				     NULL);				\
		return NULL;						\
	}

#define check_class_object(x)						\
	if (!vm_object_is_instance_of((x), vm_java_lang_Class))		\
		return NULL;

static jclass vm_jni_find_class(struct vm_jni_env *env, const char *name)
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

static jmethodID vm_jni_get_method_id(struct vm_jni_env *env, jclass clazz,
			       const char *name, const char *sig)
{
	struct vm_method *mb;
	struct vm_class *class;

	check_null(clazz);
	check_class_object(clazz);

	class = vm_class_get_class_from_class_object(clazz);
	check_null(class);

	vm_class_ensure_init(class);
	if (exception_occurred())
		return NULL;

	/* XXX: Make sure it's not static. */
	mb = vm_class_get_method(class, name, sig);
	if (!mb) {
		signal_new_exception(vm_java_lang_NoSuchMethodError, NULL);
		return NULL;
	}

	return mb;
}

static jfieldID vm_jni_get_field_id(struct vm_jni_env *env, jclass clazz,
			     const char *name, const char *sig)
{
	struct vm_field *fb;
	struct vm_class *class;

	check_null(clazz);
	check_class_object(clazz);

	class = vm_class_get_class_from_class_object(clazz);
	check_null(class);

	vm_class_ensure_init(class);
	if (exception_occurred())
		return NULL;

	/* XXX: Make sure it's not static. */
	fb = vm_class_get_field(class, name, sig);
	if (!fb) {
		signal_new_exception(vm_java_lang_NoSuchFieldError, NULL);
		return NULL;
	}

	return fb;
}

static jmethodID vm_jni_get_static_method_id(struct vm_jni_env *env,
	jclass clazz, const char *name, const char *sig)
{
	struct vm_method *mb;
	struct vm_class *class;

	check_null(clazz);

	class = vm_class_get_class_from_class_object(clazz);
	check_null(class);

	vm_class_ensure_init(class);
	if (exception_occurred())
		return NULL;

	/* XXX: Make sure it's actually static. */
	mb = vm_class_get_method(class, name, sig);
	if (!mb) {
		signal_new_exception(vm_java_lang_NoSuchMethodError, NULL);
		return NULL;
	}

	return mb;
}

static const jbyte* vm_jni_get_string_utf_chars(struct vm_jni_env *env, jobject string,
					 jboolean *is_copy)
{
	jbyte *array;

	if (!string)
		return NULL;

	array = (jbyte *) vm_string_to_cstr(string);
	if (!array)
		return NULL;

	if (is_copy)
		*is_copy = true;

	return array;
}

static void vm_release_string_utf_chars(struct vm_jni_env *env, jobject string,
				 const char *utf)
{
	free((char *)utf);
}

static jint vm_jni_throw(struct vm_jni_env *env, jthrowable exception)
{
	if (!vm_object_is_instance_of(exception, vm_java_lang_Throwable))
		return -1;

	signal_exception(exception);
	return 0;
}

static jint vm_jni_throw_new(struct vm_jni_env *env, jclass clazz,
			     const char *message)
{
	struct vm_class *class;

	if (!clazz)
		return -1;

	if (!vm_object_is_instance_of(clazz, vm_java_lang_Class))
		return -1;

	class = vm_class_get_class_from_class_object(clazz);

	return signal_new_exception(class, message);
}

static jthrowable vm_jni_exception_occurred(struct vm_jni_env *env)
{
	return exception_occurred();
}

static void vm_jni_exception_describe(struct vm_jni_env *env)
{
	if (exception_occurred())
		vm_print_exception(exception_occurred());
}

static void vm_jni_exception_clear(struct vm_jni_env *env)
{
	clear_exception();
}

static void vm_jni_fatal_error(struct vm_jni_env *env, const char *msg)
{
	die("%s", msg);
}

static void vm_jni_call_static_void_method(struct vm_jni_env *env, jclass clazz,
					   jmethodID methodID, ...)
{
	va_list args;

	va_start(args, methodID);
	vm_call_method_v(methodID, args);
	va_end(args);
}

static void
vm_jni_call_static_void_method_v(struct vm_jni_env *env, jclass clazz,
				 jmethodID methodID, va_list args)
{
	vm_call_method_v(methodID, args);
}

static jobject vm_jni_call_static_object_method(struct vm_jni_env *env,
					jclass clazz, jmethodID methodID, ...)
{
	jobject result;
	va_list args;

	va_start(args, methodID);
	result = (jobject) vm_call_method_v(methodID, args);
	va_end(args);

	return result;
}

static jobject
vm_jni_call_static_object_method_v(struct vm_jni_env *env, jclass clazz,
				   jmethodID methodID, va_list args)
{
	return (jobject) vm_call_method_v(methodID, args);
}

static jbyte vm_jni_call_static_byte_method(struct vm_jni_env *env,
				jclass clazz, jmethodID methodID, ...)
{
	jbyte result;
	va_list args;

	va_start(args, methodID);
	result = (jbyte) vm_call_method_v(methodID, args);
	va_end(args);

	return result;
}

static jbyte
vm_jni_call_static_byte_method_v(struct vm_jni_env *env, jclass clazz,
				 jmethodID methodID, va_list args)
{
	return (jbyte) vm_call_method_v(methodID, args);
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
	vm_jni_throw,
	vm_jni_throw_new,

	/* 15 */
	vm_jni_exception_occurred,
	vm_jni_exception_describe,
	vm_jni_exception_clear,
	vm_jni_fatal_error,
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
	vm_jni_get_static_method_id,
	vm_jni_call_static_object_method,

	/* 115 */
	vm_jni_call_static_object_method_v,
	NULL, /* CallStaticObjectMethodA */
	NULL, /* CallStaticBooleanMethod */
	NULL, /* CallStaticBooleanMethodV */
	NULL, /* CallStaticBooleanMethodA */

	/* 120 */
	vm_jni_call_static_byte_method,
	vm_jni_call_static_byte_method_v,
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
	vm_jni_call_static_void_method,
	vm_jni_call_static_void_method_v,
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
	vm_jni_get_string_utf_chars,

	/* 170 */
	vm_release_string_utf_chars,
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

	die("JNI handler for index %d not implemented.", index);
	return true;
}
