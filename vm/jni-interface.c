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

#include "lib/guard-page.h"

#include "vm/call.h"
#include "vm/class.h"
#include "vm/classloader.h"
#include "vm/die.h"
#include "vm/errors.h"
#include "vm/jni.h"
#include "vm/method.h"
#include "vm/object.h"
#include "vm/preload.h"
#include "vm/reflection.h"
#include "vm/stack-trace.h"
#include "vm/reference.h"

#define check_null(x)							\
	if ((x) == NULL) {						\
		signal_new_exception(vm_java_lang_NullPointerException, \
				     NULL);				\
		return NULL;						\
	}

#define check_class_object(x)						\
	if (!vm_object_is_instance_of((x), vm_java_lang_Class))		\
		return NULL;

#define JNI_NOT_IMPLEMENTED die("not implemented")

static inline void pack_args(struct vm_method *vmm, unsigned long *packed_args,
			     const jvalue *args)
{
#ifdef CONFIG_32_BIT
	struct vm_method_arg *arg;
	int packed_idx;
	int idx;

	packed_idx = 0;
	idx = 0;

	list_for_each_entry(arg, &vmm->args, list_node) {
		switch (arg->type_info.vm_type) {
		case J_BOOLEAN:
			packed_args[packed_idx++] = args[idx++].z;
			break;
		case J_BYTE:
			packed_args[packed_idx++] = args[idx++].b;
			break;
		case J_CHAR:
			packed_args[packed_idx++] = args[idx++].c;
			break;
		case J_SHORT:
			packed_args[packed_idx++] = args[idx++].s;
			break;
		case J_INT:
			packed_args[packed_idx++] = args[idx++].i;
			break;
		case J_LONG:
			packed_args[packed_idx++] = low_64(args[idx].j);
			packed_args[packed_idx++] = high_64(args[idx++].j);
			break;
		case J_FLOAT:
			packed_args[packed_idx++] = low_64(*((uint64_t *) &args[idx++]));
			break;
		case J_DOUBLE:
			packed_args[packed_idx++] = low_64(*((uint64_t *) &args[idx]));
			packed_args[packed_idx++] = high_64(*((uint64_t *) &args[idx++]));
			break;
		case J_REFERENCE:
			packed_args[packed_idx++] = (unsigned long) args[idx++].l;
			break;
		default:
			assert(!"unsupported type");
			break;
		}
	}
#else
	int count;

	count = count_java_arguments(vmm);
	memcpy(packed_args, args, sizeof(uint64_t) * count);
#endif
}

/**
 * When given method is a private methods or constructor, the method
 * must be derived from the real class of object, not from one of its
 * superclasses. It's used by CallXXXMethod() function family.
 */
static int transform_method_for_call(jobject object, jmethodID *method_p)
{
	struct vm_method *vmm = *method_p;

	if (!vm_method_is_private(vmm) && !vm_method_is_constructor(vmm))
		return 0;

	if (!object) {
		throw_npe();
		return -1;
	}

	struct vm_class *actual_class = object->class;

	if (!vm_class_is_assignable_from(vmm->class, actual_class)) {
		signal_new_exception(vm_java_lang_Error,
				     "Object not assignable to %s",
				     vmm->class->name);
		return -1;
	}

	vmm = vm_class_get_method_recursive(actual_class, vmm->name, vmm->type);
	if (!vmm) {
		signal_new_exception(vm_java_lang_NoSuchMethodError, "%s%s in %s",
				     (*method_p)->name, (*method_p)->type,
				     actual_class->name);
		return -1;
	}

	*method_p = vmm;
	return 0;
}

static int transform_method_for_nonvirtual_call(jobject this, jclass clazz,
						jmethodID *method_p)
{
	struct vm_method *vmm = *method_p;

	if (!this || !clazz || !vmm) {
		throw_npe();
		return -1;
	}

	struct vm_class *vmc =
		vm_class_get_class_from_class_object(clazz);

	if (!vmc) {
		throw_npe();
		return -1;
	}

	if (!vm_class_is_assignable_from(vmm->class, this->class)) {
		signal_new_exception(vm_java_lang_Error,
				     "Object is not assignable to %s",
				     vmm->class->name);
		return -1; /* rethrow */
	}

	vmm = vm_class_get_method_recursive(vmc, vmm->name, vmm->type);
	if (!vmm) {
		signal_new_exception(vm_java_lang_NoSuchMethodError, "%s%s",
				     (*method_p)->name, (*method_p)->type);
		return -1; /* rethrow */
	}

	*method_p = vmm;
	return 0;
}

static jfieldID
vm_jni_common_get_field_id(jclass clazz, const char *name, const char *sig)
{
	struct vm_class *class;
	struct vm_field *fb;

	check_null(clazz);
	check_class_object(clazz);

	class = vm_class_get_class_from_class_object(clazz);
	check_null(class);

	vm_class_ensure_init(class);
	if (exception_occurred())
		return NULL;

	fb = vm_class_get_field_recursive(class, name, sig);
	if (!fb) {
		signal_new_exception(vm_java_lang_NoSuchFieldError, NULL);
		return NULL;
	}

	return fb;
}

static jint JNI_DestroyJavaVM(JavaVM *vm)
{
	NOT_IMPLEMENTED;
	return 0;
}

static jint JNI_AttachCurrentThread(JavaVM *vm, void **penv, void *args)
{
	NOT_IMPLEMENTED;
	return 0;
}

static jint JNI_DetachCurrentThread(JavaVM *vm)
{
	NOT_IMPLEMENTED;
	return 0;
}

// FIXME: implicit version dependant env setting seems dangerous
static jint JNI_GetEnv(JavaVM *vm, void **env, jint version)
{
	enter_vm_from_jni();

	if (version > JNI_VERSION_1_4) {
		*env = NULL;
		return JNI_EVERSION;
	}

	*env = vm_jni_get_jni_env();
	return JNI_OK;
}

static jint JNI_AttachCurrentThreadAsDaemon(JavaVM *vm, void **penv, void *args)
{
	NOT_IMPLEMENTED;
	return 0;
}

const struct JNIInvokeInterface_ defaultJNIInvokeInterface =
{
    .reserved0 = NULL,
    .reserved1 = NULL,
    .reserved2 = NULL,
    .DestroyJavaVM = JNI_DestroyJavaVM,
    .AttachCurrentThread = JNI_AttachCurrentThread,
    .DetachCurrentThread = JNI_DetachCurrentThread,
    .GetEnv = JNI_GetEnv,
    .AttachCurrentThreadAsDaemon = JNI_AttachCurrentThreadAsDaemon,
};

const struct JNIInvokeInterface_* vm_jni_default_invoke_interface = &defaultJNIInvokeInterface;
const struct JNIInvokeInterface_** vm_jni_default_invoke_interface_ptr = &vm_jni_default_invoke_interface;

JavaVM *vm_jni_get_current_java_vm(void)
{
	return (JavaVM *) vm_jni_default_invoke_interface_ptr;
}

static jint JNI_GetVersion(JNIEnv *env)
{
	return JNI_VERSION_1_6;
}

static jclass JNI_DefineClass(JNIEnv *env, const char *name, jobject loader,const jbyte *buf, jsize bufLen)
{
	JNI_NOT_IMPLEMENTED;
	return 0;
}

static jclass JNI_FindClass(JNIEnv *env, const char *name)
{
	struct vm_class *class;

	enter_vm_from_jni();

	/* XXX: which classloader should we use here? */
	class = classloader_load(NULL, name);
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

static jmethodID JNI_FromReflectedMethod(JNIEnv *env, jobject method)
{
	return vm_object_to_vm_method(method);
}

static jfieldID JNI_FromReflectedField(JNIEnv *env, jobject field)
{
	JNI_NOT_IMPLEMENTED;
	return 0;
}

static jobject JNI_ToReflectedMethod(JNIEnv *env, jclass cls,jmethodID methodID, jboolean isStatic)
{
	JNI_NOT_IMPLEMENTED;
	return 0;
}

static jclass JNI_GetSuperclass(JNIEnv *env, jclass clazz)
{
	JNI_NOT_IMPLEMENTED;
	return 0;
}

static jboolean JNI_IsAssignableFrom(JNIEnv *env, jclass clazz1, jclass clazz2)
{
	struct vm_class *class1, *class2;

	enter_vm_from_jni();

	if (!vm_object_is_instance_of(clazz1, vm_java_lang_Class) ||
	    !vm_object_is_instance_of(clazz2, vm_java_lang_Class))
		return JNI_FALSE;

	class1 = vm_class_get_class_from_class_object(clazz1);
	class2 = vm_class_get_class_from_class_object(clazz2);

	return vm_class_is_assignable_from(class2, class1);
}

static jobject JNI_ToReflectedField(JNIEnv *env, jclass cls, jfieldID fieldID, jboolean isStatic)
{
	JNI_NOT_IMPLEMENTED;
	return 0;
}

static jint JNI_Throw(JNIEnv *env, jthrowable exception)
{
	enter_vm_from_jni();

	if (!vm_object_is_instance_of(exception, vm_java_lang_Throwable))
		return -1;

	signal_exception(exception);
	return 0;
}

static jint JNI_ThrowNew(JNIEnv *env, jclass clazz, const char *message)
{
	struct vm_class *class;

	enter_vm_from_jni();

	if (!clazz)
		return -1;

	if (!vm_object_is_instance_of(clazz, vm_java_lang_Class))
		return -1;

	class = vm_class_get_class_from_class_object(clazz);

	signal_new_exception(class, message);

	return 0;
}

static jthrowable JNI_ExceptionOccurred(JNIEnv *env)
{
	enter_vm_from_jni();

	return exception_occurred();
}

static void JNI_ExceptionDescribe(JNIEnv *env)
{
	enter_vm_from_jni();

	if (exception_occurred())
		vm_print_exception(exception_occurred());
}

static void JNI_ExceptionClear(JNIEnv *env)
{
	enter_vm_from_jni();

	clear_exception();
}

static void JNI_FatalError(JNIEnv *env, const char *msg)
{
	enter_vm_from_jni();

	die("%s", msg);
}

static jint JNI_PushLocalFrame(JNIEnv *env, jint capacity)
{
	JNI_NOT_IMPLEMENTED;
	return 0;
}

static jobject JNI_PopLocalFrame(JNIEnv *env, jobject result)
{
	JNI_NOT_IMPLEMENTED;
	return 0;
}

static jobject JNI_NewGlobalRef(JNIEnv *env, jobject obj)
{
	enter_vm_from_jni();

	/* TODO: fix this when GC is implemented. */

	return obj;
}

static void JNI_DeleteGlobalRef(JNIEnv *env, jobject globalRef)
{
	enter_vm_from_jni();

	/* TODO: fix this when GC is implemented. */
}

static void JNI_DeleteLocalRef(JNIEnv *env, jobject localRef)
{
	enter_vm_from_jni();

	/* TODO: fix this when GC is implemented. */
}

static jboolean JNI_IsSameObject(JNIEnv *env, jobject ref1, jobject ref2)
{
	enter_vm_from_jni();

	if (ref1 == ref2)
		return JNI_TRUE;

	return JNI_FALSE;
}

static jobject JNI_AllocObject(JNIEnv *env, jclass clazz)
{
	struct vm_class *class = vm_class_get_class_from_class_object(clazz);
	check_null(class);

	return vm_object_alloc(class);
}

static jobject JNI_NewLocalRef(JNIEnv *env, jobject ref)
{
	JNI_NOT_IMPLEMENTED;
	return 0;
}

static jint JNI_EnsureLocalCapacity(JNIEnv *env, jint capacity)
{
	JNI_NOT_IMPLEMENTED;
	return 0;
}

static jobject JNI_NewObject(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	va_list args;

	enter_vm_from_jni();

	struct vm_object *obj;
	struct vm_class *class;

	if (!vm_object_is_instance_of(clazz, vm_java_lang_Class))
		return NULL;

	class = vm_class_get_class_from_class_object(clazz);
	obj = vm_object_alloc(class);

	va_start(args, methodID);
	vm_call_method_this_v(methodID, obj, args, NULL);
	va_end(args);

	return obj;
}

static jobject JNI_NewObjectV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	JNI_NOT_IMPLEMENTED;
	return 0;
}

static jobject JNI_NewObjectA(JNIEnv *env, jclass clazz, jmethodID methodID, const jvalue *args)
{
	struct vm_class *vmc;
	struct vm_object *result;

	enter_vm_from_jni();

	vmc = vm_class_get_class_from_class_object(clazz);
	result = vm_object_alloc(vmc);

	unsigned long packed_args[methodID->args_count];

	packed_args[0] = (unsigned long) result;
	pack_args(methodID, packed_args + 1, args);

	vm_call_method_this_a(methodID, result, packed_args, NULL);

	return result;
}

static jclass JNI_GetObjectClass(JNIEnv *env, jobject obj)
{
	enter_vm_from_jni();

	return obj->class->object;
}

static jboolean JNI_IsInstanceOf(JNIEnv *env, jobject obj, jclass clazz)
{
	JNI_NOT_IMPLEMENTED;
	return 0;
}

static jmethodID JNI_GetMethodID(JNIEnv *env, jclass clazz, const char *name, const char *sig)
{
	struct vm_method *mb;
	struct vm_class *class;

	enter_vm_from_jni();

	check_null(clazz);
	check_class_object(clazz);

	class = vm_class_get_class_from_class_object(clazz);
	check_null(class);

	vm_class_ensure_init(class);
	if (exception_occurred())
		return NULL;

	/* XXX: Make sure it's not static. */
	mb = vm_class_get_method_recursive(class, name, sig);
	if (!mb) {
		signal_new_exception(vm_java_lang_NoSuchMethodError, NULL);
		return NULL;
	}

	return mb;
}

#define DECLARE_CALL_XXX_METHOD(type, typename, symbol)				\
	static j ## type JNI_Call ## typename ## Method		\
	(JNIEnv *env, jobject this, jmethodID methodID, ...)	\
	{								\
		va_list args;						\
		union jvalue result;					\
									\
		enter_vm_from_jni();					\
									\
		if (transform_method_for_call(this, &methodID))		\
			return 0;					\
									\
		va_start(args, methodID);				\
		vm_call_method_this_v(methodID, this, args, &result);	\
		va_end(args);						\
									\
		return result.symbol;					\
	}

DECLARE_CALL_XXX_METHOD(boolean, Boolean, z);
DECLARE_CALL_XXX_METHOD(byte, Byte, b);
DECLARE_CALL_XXX_METHOD(char, Char, c);
DECLARE_CALL_XXX_METHOD(short, Short, s);
DECLARE_CALL_XXX_METHOD(int, Int, i);
DECLARE_CALL_XXX_METHOD(long, Long, j);
DECLARE_CALL_XXX_METHOD(float, Float, f);
DECLARE_CALL_XXX_METHOD(double, Double, d);
DECLARE_CALL_XXX_METHOD(object, Object, l);

#define DECLARE_CALL_XXX_METHOD_V(type, typename, symbol)				\
        static j ## type JNI_Call ## typename ## MethodV		\
        (JNIEnv *env, jobject this, jmethodID methodID, va_list args) \
	{								\
		union jvalue result;                                    \
									\
		enter_vm_from_jni();                                    \
									\
		if (transform_method_for_call(this, &methodID))         \
			return 0;                                       \
									\
		vm_call_method_this_v(methodID, this, args, &result);   \
									\
		return result.symbol;                                   \
	}

DECLARE_CALL_XXX_METHOD_V(boolean, Boolean, z);
DECLARE_CALL_XXX_METHOD_V(byte, Byte, b);
DECLARE_CALL_XXX_METHOD_V(char, Char, c);
DECLARE_CALL_XXX_METHOD_V(short, Short, s);
DECLARE_CALL_XXX_METHOD_V(int, Int, i);
DECLARE_CALL_XXX_METHOD_V(long, Long, j);
DECLARE_CALL_XXX_METHOD_V(float, Float, f);
DECLARE_CALL_XXX_METHOD_V(double, Double, d);
DECLARE_CALL_XXX_METHOD_V(object, Object, l);

#define DECLARE_CALL_XXX_METHOD_A(type, typename, symbol)				\
        static j ## type JNI_Call ## typename ## MethodA		\
        (JNIEnv *env, jobject this, jmethodID methodID, const jvalue *args) \
	{								\
	  JNI_NOT_IMPLEMENTED; 		\
	  return 0; \
	}

DECLARE_CALL_XXX_METHOD_A(boolean, Boolean, z);
DECLARE_CALL_XXX_METHOD_A(byte, Byte, b);
DECLARE_CALL_XXX_METHOD_A(char, Char, c);
DECLARE_CALL_XXX_METHOD_A(short, Short, s);
DECLARE_CALL_XXX_METHOD_A(int, Int, i);
DECLARE_CALL_XXX_METHOD_A(long, Long, j);
DECLARE_CALL_XXX_METHOD_A(float, Float, f);
DECLARE_CALL_XXX_METHOD_A(double, Double, d);
DECLARE_CALL_XXX_METHOD_A(object, Object, l);

static void JNI_CallVoidMethod(JNIEnv *env, jobject this, jmethodID methodID, ...)
{
	va_list args;

	enter_vm_from_jni();

	if (transform_method_for_call(this, &methodID))
		return;

	va_start(args, methodID);
	vm_call_method_this_v(methodID, this, args, NULL);
	va_end(args);
}

static void JNI_CallVoidMethodV(JNIEnv *env, jobject this, jmethodID methodID, va_list args)
{
	enter_vm_from_jni();

	if (transform_method_for_call(this, &methodID))
		return;

	vm_call_method_this_v(methodID, this, args, NULL);
}

static void JNI_CallVoidMethodA(JNIEnv *env, jobject this, jmethodID methodID, const jvalue *args)
{
	enter_vm_from_jni();

	if (transform_method_for_call(this, &methodID))
		return;

	unsigned long packed_args[methodID->args_count];

	packed_args[0] = (unsigned long) this;
	pack_args(methodID, packed_args + 1, args);

	vm_call_method_this_a(methodID, this, packed_args, NULL);
}

#define DECLARE_CALL_NONVIRTUAL_XXX_METHOD(type, typename, symbol)		\
	static j ## type JNI_CallNonvirtual ## typename ## Method	\
	(JNIEnv *env, jobject this, jclass clazz, jmethodID methodID, ...)					\
	{								\
		va_list args;						\
		union jvalue result;					\
									\
		enter_vm_from_jni();					\
									\
		if (transform_method_for_nonvirtual_call(this, clazz,	\
							 &methodID))	\
			return 0; /* rethrow */				\
									\
									\
		va_start(args, methodID);				\
		vm_call_method_this_v(methodID, this, args, &result);	\
		va_end(args);						\
									\
		return result.symbol;					\
	}

DECLARE_CALL_NONVIRTUAL_XXX_METHOD(boolean, Boolean, z);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD(byte, Byte, b);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD(char, Char, c);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD(short, Short, s);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD(int, Int, i);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD(long, Long, j);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD(float, Float, f);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD(double, Double, d);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD(object, Object, l);

static void JNI_CallNonvirtualVoidMethod(JNIEnv *env, jobject this, jclass clazz, jmethodID methodID, ...)
{
	va_list args;

	enter_vm_from_jni();

	if (transform_method_for_nonvirtual_call(this, clazz, &methodID))
		return; /* rethrow */

	va_start(args, methodID);
	vm_call_method_this_v(methodID, this, args, NULL);
	va_end(args);
}

#define DECLARE_CALL_NONVIRTUAL_XXX_METHOD_A(type, typename, symbol)		\
	static j ## type JNI_CallNonvirtual ## typename ## MethodA	\
	(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, const jvalue *args)					\
	{								\
	  JNI_NOT_IMPLEMENTED; 		\
	  return 0; \
	}

DECLARE_CALL_NONVIRTUAL_XXX_METHOD_A(boolean, Boolean, z);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD_A(byte, Byte, b);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD_A(char, Char, c);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD_A(short, Short, s);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD_A(int, Int, i);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD_A(long, Long, j);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD_A(float, Float, f);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD_A(double, Double, d);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD_A(object, Object, l);

static void JNI_CallNonvirtualVoidMethodA(JNIEnv *env, jobject this, jclass clazz, jmethodID methodID, const jvalue *args)
{
	enter_vm_from_jni();

	if (transform_method_for_nonvirtual_call(this, clazz, &methodID))
		return; /* rethrow */

	unsigned long packed_args[methodID->args_count];

	packed_args[0] = (unsigned long) this;
	pack_args(methodID, packed_args + 1, args);

	vm_call_method_this_a(methodID, this, packed_args, NULL);
}

#define DECLARE_CALL_NONVIRTUAL_XXX_METHOD_V(type, typename, symbol)		\
	static j ## type JNI_CallNonvirtual ## typename ## MethodV	\
	(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)					\
	{								\
	  JNI_NOT_IMPLEMENTED; 		\
	  return 0; \
	}

DECLARE_CALL_NONVIRTUAL_XXX_METHOD_V(boolean, Boolean, z);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD_V(byte, Byte, b);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD_V(char, Char, c);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD_V(short, Short, s);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD_V(int, Int, i);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD_V(long, Long, j);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD_V(float, Float, f);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD_V(double, Double, d);
DECLARE_CALL_NONVIRTUAL_XXX_METHOD_V(object, Object, l);

static void JNI_CallNonvirtualVoidMethodV(JNIEnv *env, jobject obj, jclass clazz, jmethodID methodID, va_list args)
{
  JNI_NOT_IMPLEMENTED;
  return;
}

static jfieldID JNI_GetFieldID(JNIEnv *env, jclass clazz, const char *name, const char *sig)
{
	struct vm_field *fb;

	enter_vm_from_jni();

	fb = vm_jni_common_get_field_id(clazz, name, sig);
	if (!fb)
		return NULL;

	if (vm_field_is_static(fb))
		return NULL;

	return fb;
}

#define DECLARE_GET_XXX_FIELD(type, typename, vmtype)				\
static j ## type							\
JNI_Get ## typename ## Field(JNIEnv *env, jobject object,	jfieldID field)				\
	{								\
	enter_vm_from_jni();						\
									\
	if (!object) {							\
	signal_new_exception(vm_java_lang_NullPointerException, NULL);	\
	return 0;							\
	}								\
									\
	if (vm_field_type(field) != vmtype) {				\
	NOT_IMPLEMENTED;						\
	return 0;							\
	}								\
									\
	return field_get_ ## type (object, field);			\
}

DECLARE_GET_XXX_FIELD(object, Object, J_REFERENCE);
DECLARE_GET_XXX_FIELD(boolean, Boolean, J_BOOLEAN);
DECLARE_GET_XXX_FIELD(byte, Byte, J_BYTE);
DECLARE_GET_XXX_FIELD(char, Char, J_CHAR);
DECLARE_GET_XXX_FIELD(short, Short, J_SHORT);
DECLARE_GET_XXX_FIELD(int, Int, J_INT);
DECLARE_GET_XXX_FIELD(long, Long, J_LONG);
DECLARE_GET_XXX_FIELD(float, Float, J_FLOAT);
DECLARE_GET_XXX_FIELD(double, Double, J_DOUBLE);

#define DECLARE_SET_XXX_FIELD(type, typename, vmtype)				\
static void								\
JNI_Set ## typename ## Field(JNIEnv *env, jobject object,	jfieldID field, j ## type value)		\
	{								\
	enter_vm_from_jni();						\
									\
	if (!object) {							\
	signal_new_exception(vm_java_lang_NullPointerException, NULL);	\
	return;								\
	}								\
									\
	if (vm_field_type(field) != vmtype)				\
		return;							\
									\
	field_set_ ## type (object, field, value);			\
}

DECLARE_SET_XXX_FIELD(object, Object, J_REFERENCE);
DECLARE_SET_XXX_FIELD(boolean, Boolean, J_BOOLEAN);
DECLARE_SET_XXX_FIELD(byte, Byte, J_BYTE);
DECLARE_SET_XXX_FIELD(char, Char, J_CHAR);
DECLARE_SET_XXX_FIELD(short, Short, J_SHORT);
DECLARE_SET_XXX_FIELD(int, Int, J_INT);
DECLARE_SET_XXX_FIELD(long, Long, J_LONG);
DECLARE_SET_XXX_FIELD(float, Float, J_FLOAT);
DECLARE_SET_XXX_FIELD(double, Double, J_DOUBLE);

static jmethodID JNI_GetStaticMethodID(JNIEnv *env, jclass clazz, const char *name, const char *sig)
{
	struct vm_method *mb;
	struct vm_class *class;

	enter_vm_from_jni();

	check_null(clazz);

	class = vm_class_get_class_from_class_object(clazz);
	check_null(class);

	vm_class_ensure_init(class);
	if (exception_occurred())
		return NULL;

	/* XXX: Make sure it's actually static. */
	mb = vm_class_get_method_recursive(class, name, sig);
	if (!mb) {
		signal_new_exception(vm_java_lang_NoSuchMethodError, NULL);
		return NULL;
	}

	return mb;
}

static void JNI_CallStaticVoidMethod(JNIEnv *env, jclass clazz, jmethodID methodID, ...)
{
	va_list args;

	enter_vm_from_jni();

	va_start(args, methodID);
	vm_call_method_v(methodID, args, NULL);
	va_end(args);
}

static void JNI_CallStaticVoidMethodV(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)
{
	enter_vm_from_jni();
	vm_call_method_v(methodID, args, NULL);
}

#define DECLARE_CALL_STATIC_XXX_METHOD(name, typename, symbol)			\
	static j ## name JNI_CallStatic ## typename ## Method		\
	(JNIEnv *env, jclass clazz,jmethodID methodID, ...)  \
	{								\
		union jvalue result;					\
		va_list args;						\
									\
		enter_vm_from_jni();					\
									\
		va_start(args, methodID);				\
		vm_call_method_v(methodID, args, &result);		\
		va_end(args);						\
									\
		return result.symbol;					\
	}

DECLARE_CALL_STATIC_XXX_METHOD(object, Object, l);
DECLARE_CALL_STATIC_XXX_METHOD(boolean, Boolean, z);
DECLARE_CALL_STATIC_XXX_METHOD(byte, Byte, b);
DECLARE_CALL_STATIC_XXX_METHOD(char, Char, c);
DECLARE_CALL_STATIC_XXX_METHOD(short, Short, s);
DECLARE_CALL_STATIC_XXX_METHOD(int, Int, i);
DECLARE_CALL_STATIC_XXX_METHOD(long, Long, j);
DECLARE_CALL_STATIC_XXX_METHOD(float, Float, f);
DECLARE_CALL_STATIC_XXX_METHOD(double, Double, d);

static void JNI_CallStaticVoidMethodA(JNIEnv *env, jclass clazz, jmethodID methodID, const jvalue *args)
{
	JNI_NOT_IMPLEMENTED;
	return;
}

#define DECLARE_CALL_STATIC_XXX_METHOD_A(name, typename, symbol)			\
	static j ## name JNI_CallStatic ## typename ## MethodA	\
	(JNIEnv *env, jclass clazz, jmethodID methodID, const jvalue *args)				\
	{								\
	  JNI_NOT_IMPLEMENTED; 		\
	  return 0; \
	}

DECLARE_CALL_STATIC_XXX_METHOD_A(object, Object, l);
DECLARE_CALL_STATIC_XXX_METHOD_A(boolean, Boolean, z);
DECLARE_CALL_STATIC_XXX_METHOD_A(byte, Byte, b);
DECLARE_CALL_STATIC_XXX_METHOD_A(char, Char, c);
DECLARE_CALL_STATIC_XXX_METHOD_A(short, Short, s);
DECLARE_CALL_STATIC_XXX_METHOD_A(int, Int, i);
DECLARE_CALL_STATIC_XXX_METHOD_A(long, Long, j);
DECLARE_CALL_STATIC_XXX_METHOD_A(float, Float, f);
DECLARE_CALL_STATIC_XXX_METHOD_A(double, Double, d);

#define DECLARE_CALL_STATIC_XXX_METHOD_V(name, typename, symbol)			\
	static j ## name JNI_CallStatic ## typename ## MethodV	\
	(JNIEnv *env, jclass clazz, jmethodID methodID, va_list args)				\
	{								\
		union jvalue result;					\
									\
		enter_vm_from_jni();					\
									\
		vm_call_method_v(methodID, args, &result);		\
		return result.symbol;					\
	}

DECLARE_CALL_STATIC_XXX_METHOD_V(object, Object, l);
DECLARE_CALL_STATIC_XXX_METHOD_V(boolean, Boolean, z);
DECLARE_CALL_STATIC_XXX_METHOD_V(byte, Byte, b);
DECLARE_CALL_STATIC_XXX_METHOD_V(char, Char, c);
DECLARE_CALL_STATIC_XXX_METHOD_V(short, Short, s);
DECLARE_CALL_STATIC_XXX_METHOD_V(int, Int, i);
DECLARE_CALL_STATIC_XXX_METHOD_V(long, Long, j);
DECLARE_CALL_STATIC_XXX_METHOD_V(float, Float, f);
DECLARE_CALL_STATIC_XXX_METHOD_V(double, Double, d);

static jfieldID JNI_GetStaticFieldID(JNIEnv *env, jclass clazz, const char *name, const char *sig)
{
	struct vm_field *fb;

	enter_vm_from_jni();

	fb = vm_jni_common_get_field_id(clazz, name, sig);
	if (!fb)
		return NULL;

	if (!vm_field_is_static(fb))
		return NULL;

	return fb;
}

#define DEFINE_GET_STATIC_FIELD(func, type, get)			\
	static type							\
	func(JNIEnv *env, jclass clazz, jfieldID fieldID)	\
	{								\
		enter_vm_from_jni();					\
									\
		return get(fieldID);					\
	}								\

DEFINE_GET_STATIC_FIELD(JNI_GetStaticObjectField, jobject, static_field_get_object);
DEFINE_GET_STATIC_FIELD(JNI_GetStaticBooleanField, jboolean, static_field_get_boolean);
DEFINE_GET_STATIC_FIELD(JNI_GetStaticByteField, jbyte, static_field_get_byte);
DEFINE_GET_STATIC_FIELD(JNI_GetStaticCharField, jchar, static_field_get_char);
DEFINE_GET_STATIC_FIELD(JNI_GetStaticShortField, jshort, static_field_get_short);
DEFINE_GET_STATIC_FIELD(JNI_GetStaticIntField, jint, static_field_get_int);
DEFINE_GET_STATIC_FIELD(JNI_GetStaticLongField, jlong, static_field_get_long);
DEFINE_GET_STATIC_FIELD(JNI_GetStaticFloatField, jfloat, static_field_get_float);
DEFINE_GET_STATIC_FIELD(JNI_GetStaticDoubleField, jdouble, static_field_get_double);

#define DEFINE_SET_STATIC_FIELD(func, type, set)			\
	static void							\
	func(JNIEnv *env, jclass clazz, jfieldID fieldID, type value)				\
	{								\
		enter_vm_from_jni();					\
									\
		set(fieldID, value);					\
	}								\

DEFINE_SET_STATIC_FIELD(JNI_SetStaticObjectField, jobject, static_field_set_object);
DEFINE_SET_STATIC_FIELD(JNI_SetStaticBooleanField, jboolean, static_field_set_boolean);
DEFINE_SET_STATIC_FIELD(JNI_SetStaticByteField, jbyte, static_field_set_byte);
DEFINE_SET_STATIC_FIELD(JNI_SetStaticCharField, jchar, static_field_set_char);
DEFINE_SET_STATIC_FIELD(JNI_SetStaticShortField, jshort, static_field_set_short);
DEFINE_SET_STATIC_FIELD(JNI_SetStaticIntField, jint, static_field_set_int);
DEFINE_SET_STATIC_FIELD(JNI_SetStaticLongField, jlong, static_field_set_long);
DEFINE_SET_STATIC_FIELD(JNI_SetStaticFloatField, jfloat, static_field_set_float);
DEFINE_SET_STATIC_FIELD(JNI_SetStaticDoubleField, jdouble, static_field_set_double);

static jstring JNI_NewString(JNIEnv *env, const jchar *unicodeChars, jsize len)
{
	JNI_NOT_IMPLEMENTED;
	return 0;
}

static jsize JNI_GetStringLength(JNIEnv *env, jstring string)
{
	union jvalue result;

	enter_vm_from_jni();

	if (string->class != vm_java_lang_String) { /* String is a final */
		signal_new_exception(vm_java_lang_IllegalArgumentException, NULL);
		return 0; /* rethrow */
	}

	vm_call_method(vm_java_lang_String_length, string, &result);

	return result.i;
}

static const jchar * JNI_GetStringChars(JNIEnv *env, jstring string, jboolean *isCopy)
{
	JNI_NOT_IMPLEMENTED;
	return 0;
}

static void JNI_ReleaseStringChars(JNIEnv *env, jstring string, const jchar *chars)
{
	JNI_NOT_IMPLEMENTED;
	return;
}

static jstring JNI_NewStringUTF(JNIEnv *env, const char *bytes)
{
	enter_vm_from_jni();

	return vm_object_alloc_string_from_c(bytes);
}

static jsize JNI_GetStringUTFLength(JNIEnv *env, jstring string)
{
	JNI_NOT_IMPLEMENTED;
	return 0;
}

static const char* JNI_GetStringUTFChars(JNIEnv *env, jstring string, jboolean *isCopy)
{
	jbyte *array;

	enter_vm_from_jni();

	if (!string)
		return NULL;

	array = (jbyte *) vm_string_to_cstr(string);
	if (!array)
		return NULL;

	if (isCopy)
		*isCopy = true;

	// Fixme: should return char*, not jbyte*
	return (char*) array;
}

static void JNI_ReleaseStringUTFChars(JNIEnv *env, jstring string, const char *utf)
{
	enter_vm_from_jni();

	free((char *)utf);
}

static jsize JNI_GetArrayLength(JNIEnv *env, jarray array)

{
	enter_vm_from_jni();

	if (!vm_class_is_array_class(array->class)) {
		warn("argument is not an array");
		return 0;
	}

	return vm_array_length(array);
}

static jobjectArray JNI_NewObjectArray(JNIEnv *env, jsize length, jclass elementClass, jobject initialElement)
{
	struct vm_object *array;
	struct vm_class *vmc;

	enter_vm_from_jni();

	check_null(elementClass);

	vmc = vm_class_get_class_from_class_object(elementClass);
	if (!vmc)
		return NULL;

	array = vm_object_alloc_array_of(vmc, length);
	if (!array)
		return NULL;

	while (length)
		array_set_field_object(array, --length, initialElement);

	return array;
}

static jobject JNI_GetObjectArrayElement(JNIEnv *env, jobjectArray array, jsize index)
{
	enter_vm_from_jni();

	if (array == NULL) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return NULL;
	}
	if (index >= vm_array_length(array)) {
		signal_new_exception(vm_java_lang_ArrayIndexOutOfBoundsException, NULL);
		return NULL;
	}

	return array_get_field_object(array, index);
}

static void JNI_SetObjectArrayElement(JNIEnv *env, jobjectArray array, jsize index, jobject value)
{
	enter_vm_from_jni();

	if (array == NULL) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return;
	}
	if (index >= vm_array_length(array)) {
		signal_new_exception(vm_java_lang_ArrayIndexOutOfBoundsException, NULL);
		return;
	}

	return array_set_field_object(array, index, value);
}

#define DECLARE_NEW_XXX_ARRAY(type, typename, arr_type)				\
static j ## type ##Array JNI_New ## typename ## Array(JNIEnv *env, jsize length)	\
{									\
	jobject result;							\
									\
	enter_vm_from_jni();						\
									\
	result = vm_object_alloc_primitive_array(arr_type, length);	\
	return result;							\
}

DECLARE_NEW_XXX_ARRAY(boolean, Boolean, T_BOOLEAN);
DECLARE_NEW_XXX_ARRAY(byte, Byte, T_BYTE);
DECLARE_NEW_XXX_ARRAY(char, Char, T_CHAR);
DECLARE_NEW_XXX_ARRAY(short, Short, T_SHORT);
DECLARE_NEW_XXX_ARRAY(int, Int, T_INT);
DECLARE_NEW_XXX_ARRAY(long, Long, T_LONG);
DECLARE_NEW_XXX_ARRAY(float, Float, T_FLOAT);
DECLARE_NEW_XXX_ARRAY(double, Double, T_DOUBLE);

// FIXME: the jobject array type should be j<primitive type>Array
#define DECLARE_GET_XXX_ARRAY_ELEMENTS(type, typename)				\
static j ## type * JNI_Get ## typename ## ArrayElements(JNIEnv *env,		\
				       jobject array,			\
				       jboolean *isCopy)		\
{									\
	j ## type *result;						\
									\
	enter_vm_from_jni();						\
									\
	if (!vm_class_is_array_class(array->class) ||			\
	    vm_class_get_array_element_class(array->class)		\
	    != vm_ ## type ## _class)					\
		return NULL;						\
									\
	result = malloc(sizeof(j ## type) * vm_array_length(array));	\
	if (!result)							\
		return NULL;						\
									\
	for (long i = 0; i < vm_array_length(array); i++)		\
		result[i] = array_get_field_##type(array, i);		\
									\
	if (isCopy)							\
		*isCopy = JNI_TRUE;					\
									\
	return result;							\
}

DECLARE_GET_XXX_ARRAY_ELEMENTS(boolean, Boolean);
DECLARE_GET_XXX_ARRAY_ELEMENTS(byte, Byte);
DECLARE_GET_XXX_ARRAY_ELEMENTS(char, Char);
DECLARE_GET_XXX_ARRAY_ELEMENTS(short, Short);
DECLARE_GET_XXX_ARRAY_ELEMENTS(int, Int);
DECLARE_GET_XXX_ARRAY_ELEMENTS(long, Long);
DECLARE_GET_XXX_ARRAY_ELEMENTS(double, Double);
DECLARE_GET_XXX_ARRAY_ELEMENTS(float, Float);

// FIXME: the jobject array type should be j<primitive type>Array
#define DECLARE_RELEASE_XXX_ARRAY_ELEMENTS(type, typename)			\
static void JNI_Release ## typename ## ArrayElements(JNIEnv *env,	\
					   jobject array,		\
					   j ## type *elems, jint mode)	\
{									\
	enter_vm_from_jni();						\
									\
	if (!vm_class_is_array_class(array->class) ||			\
	    vm_class_get_array_element_class(array->class)		\
	    != vm_ ## type ## _class)					\
		return;							\
									\
	if (mode == 0 || mode == JNI_COMMIT) { /* copy back */		\
		for (long i = 0; i < vm_array_length(array); i++)	\
			array_set_field_ ## type(array, i, elems[i]);	\
	}								\
									\
	if (mode == JNI_ABORT) /* free buffer */			\
		free(elems);						\
}

DECLARE_RELEASE_XXX_ARRAY_ELEMENTS(boolean, Boolean);
DECLARE_RELEASE_XXX_ARRAY_ELEMENTS(byte, Byte);
DECLARE_RELEASE_XXX_ARRAY_ELEMENTS(char, Char);
DECLARE_RELEASE_XXX_ARRAY_ELEMENTS(short, Short);
DECLARE_RELEASE_XXX_ARRAY_ELEMENTS(int, Int);
DECLARE_RELEASE_XXX_ARRAY_ELEMENTS(long, Long);
DECLARE_RELEASE_XXX_ARRAY_ELEMENTS(float, Float);
DECLARE_RELEASE_XXX_ARRAY_ELEMENTS(double, Double);

// FIXME: the jobject array type should be j<primitive type>Array
#define DECLARE_GET_XXX_ARRAY_REGION(type, typename)				\
static void	JNI_Get ## typename ## ArrayRegion(JNIEnv *env,		\
				      jobject array,			\
				      jsize start,			\
				      jsize len,			\
				      j ## type *buf)			\
{									\
	enter_vm_from_jni();						\
									\
	if (!vm_class_is_array_class(array->class) ||			\
	    vm_class_get_array_element_class(array->class)		\
	    != vm_ ## type ## _class)					\
		return;							\
									\
	if (start < 0 || len < 0 || start + len > vm_array_length(array)) { \
		signal_new_exception(vm_java_lang_ArrayIndexOutOfBoundsException, \
				     NULL);				\
		return;							\
	}								\
									\
	for (long i = 0; i < len; i++)					\
		buf[i] = array_get_field_##type(array, start + i);	\
}

DECLARE_GET_XXX_ARRAY_REGION(boolean, Boolean);
DECLARE_GET_XXX_ARRAY_REGION(byte, Byte);
DECLARE_GET_XXX_ARRAY_REGION(char, Char);
DECLARE_GET_XXX_ARRAY_REGION(short, Short);
DECLARE_GET_XXX_ARRAY_REGION(int, Int);
DECLARE_GET_XXX_ARRAY_REGION(long, Long);
DECLARE_GET_XXX_ARRAY_REGION(float, Float);
DECLARE_GET_XXX_ARRAY_REGION(double, Double);

// FIXME: the jobject array type should be j<primitive type>Array
#define DECLARE_SET_XXX_ARRAY_REGION(type, typename)				\
static void JNI_Set ## typename ## ArrayRegion(JNIEnv *env,		\
				      jobject array,			\
				      jsize start,			\
				      jsize len,			\
				      const j ## type *buf)			\
{									\
	enter_vm_from_jni();						\
									\
	if (!vm_class_is_array_class(array->class) ||			\
	    vm_class_get_array_element_class(array->class)		\
	    != vm_ ## type ## _class)					\
		return;							\
									\
	if (start < 0 || len < 0 || start + len > vm_array_length(array)) { \
		signal_new_exception(vm_java_lang_ArrayIndexOutOfBoundsException, \
				     NULL);				\
		return;							\
	}								\
									\
	for (long i = 0; i < len; i++)					\
		array_set_field_##type(array, start + i, buf[i]);	\
}

DECLARE_SET_XXX_ARRAY_REGION(boolean, Boolean);
DECLARE_SET_XXX_ARRAY_REGION(byte, Byte);
DECLARE_SET_XXX_ARRAY_REGION(char, Char);
DECLARE_SET_XXX_ARRAY_REGION(short, Short);
DECLARE_SET_XXX_ARRAY_REGION(int, Int);
DECLARE_SET_XXX_ARRAY_REGION(long, Long);
DECLARE_SET_XXX_ARRAY_REGION(float, Float);
DECLARE_SET_XXX_ARRAY_REGION(double, Double);

static jint JNI_RegisterNatives(JNIEnv *env, jclass clazz, const JNINativeMethod *methods, jint nMethods)
{
	JNI_NOT_IMPLEMENTED;
	return 0;
}

static jint JNI_UnregisterNatives(JNIEnv *env, jclass clazz)
{
	JNI_NOT_IMPLEMENTED;
	return 0;
}

static jint JNI_MonitorEnter(JNIEnv *env, jobject obj)
{
	enter_vm_from_jni();

	int err = vm_object_lock(obj);

	if (exception_occurred())
		clear_exception();

	return err;
}

static jint JNI_MonitorExit(JNIEnv *env, jobject obj)
{
	enter_vm_from_jni();

	int err = vm_object_unlock(obj);

	if (exception_occurred())
		clear_exception();

	return err;
}

static jint JNI_GetJavaVM(JNIEnv *env, JavaVM **vm)
{
	enter_vm_from_jni();

	*vm = vm_jni_get_current_java_vm();
	return 0;
}

static void JNI_GetStringRegion(JNIEnv *env, jstring str, jsize start, jsize len, jchar *buf)
{
	JNI_NOT_IMPLEMENTED;
	return;
}

static void JNI_GetStringUTFRegion(JNIEnv *env, jstring str, jsize start, jsize len, char *buf)
{
	JNI_NOT_IMPLEMENTED;
	return;
}

#define DECLARE_GET_XXX_ARRAY_CRITICAL(type)				\
static void *								\
get_ ## type ## _array_critical (jobject array, jboolean *is_copy)	\
{									\
	j ## type *result;						\
									\
	result = malloc(sizeof(j ## type) * vm_array_length(array));	\
	if (!result)							\
		return NULL;						\
									\
	for (long i = 0; i < vm_array_length(array); i++)		\
		result[i] = array_get_field_##type(array, i);		\
									\
	if (is_copy)							\
		*is_copy = JNI_TRUE;					\
									\
	return result;							\
}

DECLARE_GET_XXX_ARRAY_CRITICAL(byte);
DECLARE_GET_XXX_ARRAY_CRITICAL(char);
DECLARE_GET_XXX_ARRAY_CRITICAL(double);
DECLARE_GET_XXX_ARRAY_CRITICAL(float);
DECLARE_GET_XXX_ARRAY_CRITICAL(int);
DECLARE_GET_XXX_ARRAY_CRITICAL(long);
DECLARE_GET_XXX_ARRAY_CRITICAL(short);
DECLARE_GET_XXX_ARRAY_CRITICAL(boolean);

typedef void *get_array_critical_fn(jobject array, jboolean *is_copy);

get_array_critical_fn *get_array_critical_fns[] = {
	[J_BYTE]	= get_byte_array_critical,
	[J_CHAR]	= get_char_array_critical,
	[J_DOUBLE]	= get_double_array_critical,
	[J_FLOAT]	= get_float_array_critical,
	[J_INT]		= get_int_array_critical,
	[J_LONG]	= get_long_array_critical,
	[J_SHORT]	= get_short_array_critical,
	[J_BOOLEAN]	= get_boolean_array_critical
};

static void * JNI_GetPrimitiveArrayCritical(JNIEnv *env, jarray array, jboolean *isCopy)
{
	enter_vm_from_jni();

	if (!vm_class_is_array_class(array->class))
		return NULL;

	struct vm_class *elem_class
		= vm_class_get_array_element_class(array->class);

	if (!elem_class)
		return rethrow_exception();

	if (!vm_class_is_primitive_class(elem_class))
		return NULL;

	enum vm_type elem_type
		= vm_class_get_storage_vmtype(elem_class);

	get_array_critical_fn *get_array_critical
		= get_array_critical_fns[elem_type];

	assert(get_array_critical);
	return get_array_critical(array, isCopy);
}

#define DECLARE_RELEASE_XXX_ARRAY_CRITICAL(type)			\
static void								\
release_ ## type ## _array_critical (jobject array, j ## type *elems, jint mode) \
{									\
	if (mode == 0 || mode == JNI_COMMIT) { /* copy back */		\
		for (long i = 0; i < vm_array_length(array); i++)	\
			array_set_field_ ## type(array, i, elems[i]);	\
	}								\
									\
	if (mode == JNI_ABORT) /* free buffer */			\
		free(elems);						\
}

DECLARE_RELEASE_XXX_ARRAY_CRITICAL(byte);
DECLARE_RELEASE_XXX_ARRAY_CRITICAL(char);
DECLARE_RELEASE_XXX_ARRAY_CRITICAL(double);
DECLARE_RELEASE_XXX_ARRAY_CRITICAL(float);
DECLARE_RELEASE_XXX_ARRAY_CRITICAL(int);
DECLARE_RELEASE_XXX_ARRAY_CRITICAL(long);
DECLARE_RELEASE_XXX_ARRAY_CRITICAL(short);
DECLARE_RELEASE_XXX_ARRAY_CRITICAL(boolean);

typedef void release_array_critical_fn(jobject array, void *carray, jint mode);

release_array_critical_fn *release_array_critical_fns[] = {
	[J_BYTE]	= (release_array_critical_fn*) release_byte_array_critical,
	[J_CHAR]	= (release_array_critical_fn*) release_char_array_critical,
	[J_DOUBLE]	= (release_array_critical_fn*) release_double_array_critical,
	[J_FLOAT]	= (release_array_critical_fn*) release_float_array_critical,
	[J_INT]		= (release_array_critical_fn*) release_int_array_critical,
	[J_LONG]	= (release_array_critical_fn*) release_long_array_critical,
	[J_SHORT]	= (release_array_critical_fn*) release_short_array_critical,
	[J_BOOLEAN]	= (release_array_critical_fn*) release_boolean_array_critical
};

// FIXME: the jobject array should be jarray array
static void JNI_ReleasePrimitiveArrayCritical(JNIEnv *env, jobject array, void *carray, jint mode)
{
	enter_vm_from_jni();

	if (!vm_class_is_array_class(array->class))
		return;

	struct vm_class *elem_class
		= vm_class_get_array_element_class(array->class);

	if (!elem_class)
		return;

	if (!vm_class_is_primitive_class(elem_class))
		return;

	enum vm_type elem_type
		= vm_class_get_storage_vmtype(elem_class);

	release_array_critical_fn *release_array_critical
		= release_array_critical_fns[elem_type];

	assert(release_array_critical);
	release_array_critical(array, carray, mode);
}

static const jchar * JNI_GetStringCritical(JNIEnv *env, jstring string, jboolean *isCopy)
{
	JNI_NOT_IMPLEMENTED;
	return 0;
}

static void JNI_ReleaseStringCritical(JNIEnv *env, jstring string, const jchar *carray)
{
	JNI_NOT_IMPLEMENTED;
	return;
}

static jweak JNI_NewWeakGlobalRef(JNIEnv *env, jobject obj)
{
	struct vm_reference *ref;

	if (!obj)
		return NULL;

	ref	= vm_reference_alloc_weak(obj);
	if (!ref)
		return throw_oom_error();

	return ref->object;
}

static void JNI_DeleteWeakGlobalRef(JNIEnv *env, jweak obj)
{
	vm_reference_collect_for_object(obj);
}

static jboolean JNI_ExceptionCheck(JNIEnv *env)
{
	return exception_occurred() != NULL;
}

static jobject JNI_NewDirectByteBuffer(JNIEnv *env, void *address, jlong capacity)
{
	struct vm_object *ret, *data;

	enter_vm_from_jni();

	ret = vm_object_alloc(vm_java_nio_DirectByteBufferImpl_ReadWrite);
	if (!ret)
		return NULL;

	data = vm_object_alloc(vm_gnu_classpath_PointerNN);
	if (!data)
		return NULL;

#ifdef CONFIG_32_BIT
	field_set_int(ret, vm_gnu_classpath_PointerNN_data, (jint) data);
#else
	field_set_long(ret, vm_gnu_classpath_PointerNN_data, (jlong) data);
#endif

	vm_call_method(vm_java_nio_DirectByteBufferImpl_ReadWrite_init, ret, NULL, data, capacity, capacity, 0);

	return ret;
}

static void *JNI_GetDirectBufferAddress(JNIEnv *env, jobject buf)
{
	struct vm_object *address;
	void *data;

	enter_vm_from_jni();

	if (buf == NULL)
		return NULL;

	address		= field_get_object(buf, vm_java_nio_Buffer_address);
	if (!address)
		return NULL;

#ifdef CONFIG_32_BIT
	data		= (void *) field_get_int(buf, vm_gnu_classpath_PointerNN_data);
#else
	data		= (void *) field_get_long(buf, vm_gnu_classpath_PointerNN_data);
#endif

	return data;
}

static jlong JNI_GetDirectBufferCapacity(JNIEnv *env, jobject buf)
{
	enter_vm_from_jni();

	error("%s is not supported", __func__);

	return 0;
}

static jobjectRefType JNI_GetObjectRefType(JNIEnv* env, jobject obj)
{
	JNI_NOT_IMPLEMENTED;
	return 0;
}

/*
 * The JNI native interface table.
 * http://download.oracle.com/javase/6/docs/technotes/guides/jni/spec/jniTOC.html
 */
void *vm_jni_native_interface[] = {
	/* 0 */
	NULL,
	NULL,
	NULL,
	NULL,
	JNI_GetVersion,

	/* 5 */
	JNI_DefineClass,
	JNI_FindClass,
	JNI_FromReflectedMethod,
	JNI_FromReflectedField,
	JNI_ToReflectedMethod,

	/* 10 */
	JNI_GetSuperclass,
	JNI_IsAssignableFrom,
	JNI_ToReflectedField,
	JNI_Throw,
	JNI_ThrowNew,

	/* 15 */
	JNI_ExceptionOccurred,
	JNI_ExceptionDescribe,
	JNI_ExceptionClear,
	JNI_FatalError,
	JNI_PushLocalFrame,

	/* 20 */
	JNI_PopLocalFrame,
	JNI_NewGlobalRef,
	JNI_DeleteGlobalRef,
	JNI_DeleteLocalRef,
	JNI_IsSameObject,

	/* 25 */
	NULL,
	NULL,
	JNI_AllocObject,
	JNI_NewObject,
	JNI_NewObjectV,

	/* 30 */
	JNI_NewObjectA,
	JNI_GetObjectClass,
	JNI_IsInstanceOf,
	JNI_GetMethodID,
	JNI_CallObjectMethod,

	/* 35 */
	JNI_CallObjectMethodV,
	JNI_CallObjectMethodA,
	JNI_CallBooleanMethod,
	JNI_CallBooleanMethodV,
	JNI_CallBooleanMethodA,

	/* 40 */
	JNI_CallByteMethod,
	JNI_CallByteMethodV,
	JNI_CallByteMethodA,
	JNI_CallCharMethod,
	JNI_CallCharMethodV,

	/* 45 */
	JNI_CallCharMethodA,
	JNI_CallShortMethod,
	JNI_CallShortMethodV,
	JNI_CallShortMethodA,
	JNI_CallIntMethod,

	/* 50 */
	JNI_CallIntMethodV,
	JNI_CallIntMethodA,
	JNI_CallLongMethod,
	JNI_CallLongMethodV,
	JNI_CallLongMethodA,

	/* 55 */
	JNI_CallFloatMethod,
	JNI_CallFloatMethodV,
	JNI_CallFloatMethodA,
	JNI_CallDoubleMethod,
	JNI_CallDoubleMethodV,

	/* 60 */
	JNI_CallDoubleMethodA,
	JNI_CallVoidMethod,
	JNI_CallVoidMethodV,
	JNI_CallVoidMethodA,
	JNI_CallNonvirtualObjectMethod,

	/* 65 */
	JNI_CallNonvirtualObjectMethodV,
	JNI_CallNonvirtualObjectMethodA,
	JNI_CallNonvirtualBooleanMethod,
	JNI_CallNonvirtualBooleanMethodV,
	JNI_CallNonvirtualBooleanMethodA,

	/* 70 */
	JNI_CallNonvirtualByteMethod,
	JNI_CallNonvirtualByteMethodV,
	JNI_CallNonvirtualByteMethodA,
	JNI_CallNonvirtualCharMethod,
	JNI_CallNonvirtualCharMethodV,

	/* 75 */
	JNI_CallNonvirtualCharMethodA,
	JNI_CallNonvirtualShortMethod,
	JNI_CallNonvirtualShortMethodV,
	JNI_CallNonvirtualShortMethodA,
	JNI_CallNonvirtualIntMethod,

	/* 80 */
	JNI_CallNonvirtualIntMethodV,
	JNI_CallNonvirtualIntMethodA,
	JNI_CallNonvirtualLongMethod,
	JNI_CallNonvirtualLongMethodV,
	JNI_CallNonvirtualLongMethodA,

	/* 85 */
	JNI_CallNonvirtualFloatMethod,
	JNI_CallNonvirtualFloatMethodV,
	JNI_CallNonvirtualFloatMethodA,
	JNI_CallNonvirtualDoubleMethod,
	JNI_CallNonvirtualDoubleMethodV,

	/* 90 */
	JNI_CallNonvirtualDoubleMethodA,
	JNI_CallNonvirtualVoidMethod,
	JNI_CallNonvirtualVoidMethodV,
	JNI_CallNonvirtualVoidMethodA,
	JNI_GetFieldID,

	/* 95 */
	JNI_GetObjectField,
	JNI_GetBooleanField,
	JNI_GetByteField,
	JNI_GetCharField,
	JNI_GetShortField,

	/* 100 */
	JNI_GetIntField,
	JNI_GetLongField,
	JNI_GetFloatField,
	JNI_GetDoubleField,
	JNI_SetObjectField,

	/* 105 */
	JNI_SetBooleanField,
	JNI_SetByteField,
	JNI_SetCharField,
	JNI_SetShortField,
	JNI_SetIntField,

	/* 110 */
	JNI_SetLongField,
	JNI_SetFloatField,
	JNI_SetDoubleField,
	JNI_GetStaticMethodID,
	JNI_CallStaticObjectMethod,

	/* 115 */
	JNI_CallStaticObjectMethodV,
	JNI_CallStaticObjectMethodA,
	JNI_CallStaticBooleanMethod,
	JNI_CallStaticBooleanMethodV,
	JNI_CallStaticBooleanMethodA,

	/* 120 */
	JNI_CallStaticByteMethod,
	JNI_CallStaticByteMethodV,
	JNI_CallStaticByteMethodA,
	JNI_CallStaticCharMethod,
	JNI_CallStaticCharMethodV,

	/* 125 */
	JNI_CallStaticCharMethodA,
	JNI_CallStaticShortMethod,
	JNI_CallStaticShortMethodV,
	JNI_CallStaticShortMethodA,
	JNI_CallStaticIntMethod,

	/* 130 */
	JNI_CallStaticIntMethodV,
	JNI_CallStaticIntMethodA,
	JNI_CallStaticLongMethod,
	JNI_CallStaticLongMethodV,
	JNI_CallStaticLongMethodA,

	/* 135 */
	JNI_CallStaticFloatMethod,
	JNI_CallStaticFloatMethodV,
	JNI_CallStaticFloatMethodA,
	JNI_CallStaticDoubleMethod,
	JNI_CallStaticDoubleMethodV,

	/* 140 */
	JNI_CallStaticDoubleMethodA,
	JNI_CallStaticVoidMethod,
	JNI_CallStaticVoidMethodV,
	JNI_CallStaticVoidMethodA,
	JNI_GetStaticFieldID,

	/* 145 */
	JNI_GetStaticObjectField,
	JNI_GetStaticBooleanField,
	JNI_GetStaticByteField,
	JNI_GetStaticCharField,
	JNI_GetStaticShortField,

	/* 150 */
	JNI_GetStaticIntField,
	JNI_GetStaticLongField,
	JNI_GetStaticFloatField,
	JNI_GetStaticDoubleField,
	JNI_SetStaticObjectField,

	/* 155 */
	JNI_SetStaticBooleanField,
	JNI_SetStaticByteField,
	JNI_SetStaticCharField,
	JNI_SetStaticShortField,
	JNI_SetStaticIntField,

	/* 160 */
	JNI_SetStaticLongField,
	JNI_SetStaticFloatField,
	JNI_SetStaticDoubleField,
	JNI_NewString,
	JNI_GetStringLength,

	/* 165 */
	JNI_GetStringChars,
	JNI_ReleaseStringChars,
	JNI_NewStringUTF,
	JNI_GetStringUTFLength,
	JNI_GetStringUTFChars,

	/* 170 */
	JNI_ReleaseStringUTFChars,
	JNI_GetArrayLength,
	JNI_NewObjectArray,
	JNI_GetObjectArrayElement,
	JNI_SetObjectArrayElement,

	/* 175 */
	JNI_NewBooleanArray,
	JNI_NewByteArray,
	JNI_NewCharArray,
	JNI_NewShortArray,
	JNI_NewIntArray,

	/* 180 */
	JNI_NewLongArray,
	JNI_NewFloatArray,
	JNI_NewDoubleArray,
	JNI_GetBooleanArrayElements,
	JNI_GetByteArrayElements,

	/* 185 */
	JNI_GetCharArrayElements,
	JNI_GetShortArrayElements,
	JNI_GetIntArrayElements,
	JNI_GetLongArrayElements,
	JNI_GetFloatArrayElements,

	/* 190 */
	JNI_GetDoubleArrayElements,
	JNI_ReleaseBooleanArrayElements,
	JNI_ReleaseByteArrayElements,
	JNI_ReleaseCharArrayElements,
	JNI_ReleaseShortArrayElements,

	/* 195 */
	JNI_ReleaseIntArrayElements,
	JNI_ReleaseLongArrayElements,
	JNI_ReleaseFloatArrayElements,
	JNI_ReleaseDoubleArrayElements,
	JNI_GetBooleanArrayRegion,

	/* 200 */
	JNI_GetByteArrayRegion,
	JNI_GetCharArrayRegion,
	JNI_GetShortArrayRegion,
	JNI_GetIntArrayRegion,
	JNI_GetLongArrayRegion,

	/* 205 */
	JNI_GetFloatArrayRegion,
	JNI_GetDoubleArrayRegion,
	JNI_SetBooleanArrayRegion,
	JNI_SetByteArrayRegion,
	JNI_SetCharArrayRegion,

	/* 210 */
	JNI_SetShortArrayRegion,
	JNI_SetIntArrayRegion,
	JNI_SetLongArrayRegion,
	JNI_SetFloatArrayRegion,
	JNI_SetDoubleArrayRegion,

	/* 215 */
	JNI_RegisterNatives,
	JNI_UnregisterNatives,
	JNI_MonitorEnter,
	JNI_MonitorExit,
	JNI_GetJavaVM,

	/* JNI 1.2 functions */

	/* 220 */
	JNI_GetStringRegion,
	JNI_GetStringUTFRegion,
	JNI_GetPrimitiveArrayCritical,
	JNI_ReleasePrimitiveArrayCritical,
	JNI_GetStringCritical,

	/* 225 */
	JNI_ReleaseStringCritical,
	JNI_NewWeakGlobalRef,
	JNI_DeleteWeakGlobalRef,
	JNI_ExceptionCheck,

	/* JNI 1.4 functions */

	JNI_NewDirectByteBuffer,

	/* 230 */
	JNI_GetDirectBufferAddress,
	JNI_GetDirectBufferCapacity,

	/* JNI 1.5 functions */
	JNI_GetObjectRefType,
};

const struct JNINativeInterface_ defaultJNIEnv =
{
	.reserved0 = NULL,
	.reserved1 = NULL,
	.reserved2 = NULL,
	.reserved3 = NULL,
	.GetVersion = JNI_GetVersion,
	.DefineClass = JNI_DefineClass,
	.FindClass = JNI_FindClass,
	.FromReflectedMethod = JNI_FromReflectedMethod,
	.FromReflectedField = JNI_FromReflectedField,
	.ToReflectedMethod = JNI_ToReflectedMethod,
	.GetSuperclass = JNI_GetSuperclass,
	.IsAssignableFrom = JNI_IsAssignableFrom,
	.ToReflectedField = JNI_ToReflectedField,
	.Throw = JNI_Throw,
	.ThrowNew = JNI_ThrowNew,
	.ExceptionOccurred = JNI_ExceptionOccurred,
	.ExceptionDescribe = JNI_ExceptionDescribe,
	.ExceptionClear = JNI_ExceptionClear,
	.FatalError = JNI_FatalError,
	.PushLocalFrame = JNI_PushLocalFrame,
	.PopLocalFrame = JNI_PopLocalFrame,
	.NewGlobalRef = JNI_NewGlobalRef,
	.DeleteGlobalRef = JNI_DeleteGlobalRef,
	.DeleteLocalRef = JNI_DeleteLocalRef,
	.IsSameObject = JNI_IsSameObject,
	.NewLocalRef = JNI_NewLocalRef,
	.EnsureLocalCapacity = JNI_EnsureLocalCapacity,
	.AllocObject = JNI_AllocObject,
	.NewObject = JNI_NewObject,
	.NewObjectV = JNI_NewObjectV,
	.NewObjectA = JNI_NewObjectA,
	.GetObjectClass = JNI_GetObjectClass,
	.IsInstanceOf = JNI_IsInstanceOf,
	.GetMethodID = JNI_GetMethodID,
	.CallObjectMethod = JNI_CallObjectMethod,
	.CallObjectMethodV = JNI_CallObjectMethodV,
	.CallObjectMethodA = JNI_CallObjectMethodA,
	.CallBooleanMethod = JNI_CallBooleanMethod,
	.CallBooleanMethodV = JNI_CallBooleanMethodV,
	.CallBooleanMethodA = JNI_CallBooleanMethodA,
	.CallByteMethod = JNI_CallByteMethod,
	.CallByteMethodV = JNI_CallByteMethodV,
	.CallByteMethodA = JNI_CallByteMethodA,
	.CallCharMethod = JNI_CallCharMethod,
	.CallCharMethodV = JNI_CallCharMethodV,
	.CallCharMethodA = JNI_CallCharMethodA,
	.CallShortMethod = JNI_CallShortMethod,
	.CallShortMethodV = JNI_CallShortMethodV,
	.CallShortMethodA = JNI_CallShortMethodA,
	.CallIntMethod = JNI_CallIntMethod,
	.CallIntMethodV = JNI_CallIntMethodV,
	.CallIntMethodA = JNI_CallIntMethodA,
	.CallLongMethod = JNI_CallLongMethod,
	.CallLongMethodV = JNI_CallLongMethodV,
	.CallLongMethodA = JNI_CallLongMethodA,
	.CallFloatMethod = JNI_CallFloatMethod,
	.CallFloatMethodV = JNI_CallFloatMethodV,
	.CallFloatMethodA = JNI_CallFloatMethodA,
	.CallDoubleMethod = JNI_CallDoubleMethod,
	.CallDoubleMethodV = JNI_CallDoubleMethodV,
	.CallDoubleMethodA = JNI_CallDoubleMethodA,
	.CallVoidMethod = JNI_CallVoidMethod,
	.CallVoidMethodV = JNI_CallVoidMethodV,
	.CallVoidMethodA = JNI_CallVoidMethodA,
	.CallNonvirtualObjectMethod = JNI_CallNonvirtualObjectMethod,
	.CallNonvirtualObjectMethodV = JNI_CallNonvirtualObjectMethodV,
	.CallNonvirtualObjectMethodA = JNI_CallNonvirtualObjectMethodA,
	.CallNonvirtualBooleanMethod = JNI_CallNonvirtualBooleanMethod,
	.CallNonvirtualBooleanMethodV = JNI_CallNonvirtualBooleanMethodV,
	.CallNonvirtualBooleanMethodA = JNI_CallNonvirtualBooleanMethodA,
	.CallNonvirtualByteMethod = JNI_CallNonvirtualByteMethod,
	.CallNonvirtualByteMethodV = JNI_CallNonvirtualByteMethodV,
	.CallNonvirtualByteMethodA = JNI_CallNonvirtualByteMethodA,
	.CallNonvirtualCharMethod = JNI_CallNonvirtualCharMethod,
	.CallNonvirtualCharMethodV = JNI_CallNonvirtualCharMethodV,
	.CallNonvirtualCharMethodA = JNI_CallNonvirtualCharMethodA,
	.CallNonvirtualShortMethod = JNI_CallNonvirtualShortMethod,
	.CallNonvirtualShortMethodV = JNI_CallNonvirtualShortMethodV,
	.CallNonvirtualShortMethodA = JNI_CallNonvirtualShortMethodA,
	.CallNonvirtualIntMethod = JNI_CallNonvirtualIntMethod,
	.CallNonvirtualIntMethodV = JNI_CallNonvirtualIntMethodV,
	.CallNonvirtualIntMethodA = JNI_CallNonvirtualIntMethodA,
	.CallNonvirtualLongMethod = JNI_CallNonvirtualLongMethod,
	.CallNonvirtualLongMethodV = JNI_CallNonvirtualLongMethodV,
	.CallNonvirtualLongMethodA = JNI_CallNonvirtualLongMethodA,
	.CallNonvirtualFloatMethod = JNI_CallNonvirtualFloatMethod,
	.CallNonvirtualFloatMethodV = JNI_CallNonvirtualFloatMethodV,
	.CallNonvirtualFloatMethodA = JNI_CallNonvirtualFloatMethodA,
	.CallNonvirtualDoubleMethod = JNI_CallNonvirtualDoubleMethod,
	.CallNonvirtualDoubleMethodV = JNI_CallNonvirtualDoubleMethodV,
	.CallNonvirtualDoubleMethodA = JNI_CallNonvirtualDoubleMethodA,
	.CallNonvirtualVoidMethod = JNI_CallNonvirtualVoidMethod,
	.CallNonvirtualVoidMethodV = JNI_CallNonvirtualVoidMethodV,
	.CallNonvirtualVoidMethodA = JNI_CallNonvirtualVoidMethodA,
	.GetFieldID = JNI_GetFieldID,
	.GetObjectField = JNI_GetObjectField,
	.GetBooleanField = JNI_GetBooleanField,
	.GetByteField = JNI_GetByteField,
	.GetCharField = JNI_GetCharField,
	.GetShortField = JNI_GetShortField,
	.GetIntField = JNI_GetIntField,
	.GetLongField = JNI_GetLongField,
	.GetFloatField = JNI_GetFloatField,
	.GetDoubleField = JNI_GetDoubleField,
	.SetObjectField = JNI_SetObjectField,
	.SetBooleanField = JNI_SetBooleanField,
	.SetByteField = JNI_SetByteField,
	.SetCharField = JNI_SetCharField,
	.SetShortField = JNI_SetShortField,
	.SetIntField = JNI_SetIntField,
	.SetLongField = JNI_SetLongField,
	.SetFloatField = JNI_SetFloatField,
	.SetDoubleField = JNI_SetDoubleField,
	.GetStaticMethodID = JNI_GetStaticMethodID,
	.CallStaticObjectMethod = JNI_CallStaticObjectMethod,
	.CallStaticObjectMethodV = JNI_CallStaticObjectMethodV,
	.CallStaticObjectMethodA = JNI_CallStaticObjectMethodA,
	.CallStaticBooleanMethod = JNI_CallStaticBooleanMethod,
	.CallStaticBooleanMethodV = JNI_CallStaticBooleanMethodV,
	.CallStaticBooleanMethodA = JNI_CallStaticBooleanMethodA,
	.CallStaticByteMethod = JNI_CallStaticByteMethod,
	.CallStaticByteMethodV = JNI_CallStaticByteMethodV,
	.CallStaticByteMethodA = JNI_CallStaticByteMethodA,
	.CallStaticCharMethod = JNI_CallStaticCharMethod,
	.CallStaticCharMethodV = JNI_CallStaticCharMethodV,
	.CallStaticCharMethodA = JNI_CallStaticCharMethodA,
	.CallStaticShortMethod = JNI_CallStaticShortMethod,
	.CallStaticShortMethodV = JNI_CallStaticShortMethodV,
	.CallStaticShortMethodA = JNI_CallStaticShortMethodA,
	.CallStaticIntMethod = JNI_CallStaticIntMethod,
	.CallStaticIntMethodV = JNI_CallStaticIntMethodV,
	.CallStaticIntMethodA = JNI_CallStaticIntMethodA,
	.CallStaticLongMethod = JNI_CallStaticLongMethod,
	.CallStaticLongMethodV = JNI_CallStaticLongMethodV,
	.CallStaticLongMethodA = JNI_CallStaticLongMethodA,
	.CallStaticFloatMethod = JNI_CallStaticFloatMethod,
	.CallStaticFloatMethodV = JNI_CallStaticFloatMethodV,
	.CallStaticFloatMethodA = JNI_CallStaticFloatMethodA,
	.CallStaticDoubleMethod = JNI_CallStaticDoubleMethod,
	.CallStaticDoubleMethodV = JNI_CallStaticDoubleMethodV,
	.CallStaticDoubleMethodA = JNI_CallStaticDoubleMethodA,
	.CallStaticVoidMethod = JNI_CallStaticVoidMethod,
	.CallStaticVoidMethodV = JNI_CallStaticVoidMethodV,
	.CallStaticVoidMethodA = JNI_CallStaticVoidMethodA,
	.GetStaticFieldID = JNI_GetStaticFieldID,
	.GetStaticObjectField = JNI_GetStaticObjectField,
	.GetStaticBooleanField = JNI_GetStaticBooleanField,
	.GetStaticByteField = JNI_GetStaticByteField,
	.GetStaticCharField = JNI_GetStaticCharField,
	.GetStaticShortField = JNI_GetStaticShortField,
	.GetStaticIntField = JNI_GetStaticIntField,
	.GetStaticLongField = JNI_GetStaticLongField,
	.GetStaticFloatField = JNI_GetStaticFloatField,
	.GetStaticDoubleField = JNI_GetStaticDoubleField,
	.SetStaticObjectField = JNI_SetStaticObjectField,
	.SetStaticBooleanField = JNI_SetStaticBooleanField,
	.SetStaticByteField = JNI_SetStaticByteField,
	.SetStaticCharField = JNI_SetStaticCharField,
	.SetStaticShortField = JNI_SetStaticShortField,
	.SetStaticIntField = JNI_SetStaticIntField,
	.SetStaticLongField = JNI_SetStaticLongField,
	.SetStaticFloatField = JNI_SetStaticFloatField,
	.SetStaticDoubleField = JNI_SetStaticDoubleField,
	.NewString = JNI_NewString,
	.GetStringLength = JNI_GetStringLength,
	.GetStringChars = JNI_GetStringChars,
	.ReleaseStringChars = JNI_ReleaseStringChars,
	.NewStringUTF = JNI_NewStringUTF,
	.GetStringUTFLength = JNI_GetStringUTFLength,
	.GetStringUTFChars = JNI_GetStringUTFChars,
	.ReleaseStringUTFChars = JNI_ReleaseStringUTFChars,
	.GetArrayLength = JNI_GetArrayLength,
	.NewObjectArray = JNI_NewObjectArray,
	.GetObjectArrayElement = JNI_GetObjectArrayElement,
	.SetObjectArrayElement = JNI_SetObjectArrayElement,
	.NewBooleanArray = JNI_NewBooleanArray,
	.NewByteArray = JNI_NewByteArray,
	.NewCharArray = JNI_NewCharArray,
	.NewShortArray = JNI_NewShortArray,
	.NewIntArray = JNI_NewIntArray,
	.NewLongArray = JNI_NewLongArray,
	.NewFloatArray = JNI_NewFloatArray,
	.NewDoubleArray = JNI_NewDoubleArray,
	.GetBooleanArrayElements = JNI_GetBooleanArrayElements,
	.GetByteArrayElements = JNI_GetByteArrayElements,
	.GetCharArrayElements = JNI_GetCharArrayElements,
	.GetShortArrayElements = JNI_GetShortArrayElements,
	.GetIntArrayElements = JNI_GetIntArrayElements,
	.GetLongArrayElements = JNI_GetLongArrayElements,
	.GetFloatArrayElements = JNI_GetFloatArrayElements,
	.GetDoubleArrayElements = JNI_GetDoubleArrayElements,
	.ReleaseBooleanArrayElements = JNI_ReleaseBooleanArrayElements,
	.ReleaseByteArrayElements = JNI_ReleaseByteArrayElements,
	.ReleaseCharArrayElements = JNI_ReleaseCharArrayElements,
	.ReleaseShortArrayElements = JNI_ReleaseShortArrayElements,
	.ReleaseIntArrayElements = JNI_ReleaseIntArrayElements,
	.ReleaseLongArrayElements = JNI_ReleaseLongArrayElements,
	.ReleaseFloatArrayElements = JNI_ReleaseFloatArrayElements,
	.ReleaseDoubleArrayElements = JNI_ReleaseDoubleArrayElements,
	.GetBooleanArrayRegion = JNI_GetBooleanArrayRegion,
	.GetByteArrayRegion = JNI_GetByteArrayRegion,
	.GetCharArrayRegion = JNI_GetCharArrayRegion,
	.GetShortArrayRegion = JNI_GetShortArrayRegion,
	.GetIntArrayRegion =   JNI_GetIntArrayRegion,
	.GetLongArrayRegion = JNI_GetLongArrayRegion,
	.GetFloatArrayRegion = JNI_GetFloatArrayRegion,
	.GetDoubleArrayRegion = JNI_GetDoubleArrayRegion,
	.SetBooleanArrayRegion = JNI_SetBooleanArrayRegion,
	.SetByteArrayRegion = JNI_SetByteArrayRegion,
	.SetCharArrayRegion = JNI_SetCharArrayRegion,
	.SetShortArrayRegion = JNI_SetShortArrayRegion,
	.SetIntArrayRegion = JNI_SetIntArrayRegion,
	.SetLongArrayRegion = JNI_SetLongArrayRegion,
	.SetFloatArrayRegion = JNI_SetFloatArrayRegion,
	.SetDoubleArrayRegion = JNI_SetDoubleArrayRegion,
	.RegisterNatives = JNI_RegisterNatives,
	.UnregisterNatives = JNI_UnregisterNatives,
	.MonitorEnter = JNI_MonitorEnter,
	.MonitorExit = JNI_MonitorExit,
	.GetJavaVM = JNI_GetJavaVM,
	.GetStringRegion = JNI_GetStringRegion,
	.GetStringUTFRegion = JNI_GetStringUTFRegion,
	.GetPrimitiveArrayCritical = JNI_GetPrimitiveArrayCritical,
	.ReleasePrimitiveArrayCritical = JNI_ReleasePrimitiveArrayCritical,
	.GetStringCritical = JNI_GetStringCritical,
	.ReleaseStringCritical = JNI_ReleaseStringCritical,
	.NewWeakGlobalRef = JNI_NewWeakGlobalRef,
	.DeleteWeakGlobalRef = JNI_DeleteWeakGlobalRef,
	.ExceptionCheck = JNI_ExceptionCheck,
	.NewDirectByteBuffer = JNI_NewDirectByteBuffer,
	.GetDirectBufferAddress = JNI_GetDirectBufferAddress,
	.GetDirectBufferCapacity = JNI_GetDirectBufferCapacity,
	.GetObjectRefType = JNI_GetObjectRefType,
};

const struct JNINativeInterface_* vm_jni_default_env = &defaultJNIEnv;
const struct JNINativeInterface_** vm_jni_default_env_ptr = &vm_jni_default_env;

JNIEnv *vm_jni_get_jni_env(void)
{
	return (JNIEnv *) vm_jni_default_env_ptr;
}

static void *jni_not_implemented_trap;

void vm_jni_init_interface(void)
{
	jni_not_implemented_trap = alloc_guard_page(true);
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

	warn("JNI handler for index %d not implemented.", index);
	return true;
}
