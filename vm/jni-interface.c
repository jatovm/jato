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
#include "vm/java-version.h"

#define DEFINE_JNI_FUNCTION(name) .name = JNI_##name

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

const struct JNIInvokeInterface_ defaultJNIInvokeInterface = {
    DEFINE_JNI_FUNCTION(DestroyJavaVM),
    DEFINE_JNI_FUNCTION(AttachCurrentThread),
    DEFINE_JNI_FUNCTION(DetachCurrentThread),
    DEFINE_JNI_FUNCTION(GetEnv),
    DEFINE_JNI_FUNCTION(AttachCurrentThreadAsDaemon),
};

const struct JNIInvokeInterface_* vm_jni_default_invoke_interface = &defaultJNIInvokeInterface;
const struct JNIInvokeInterface_** vm_jni_default_invoke_interface_ptr = &vm_jni_default_invoke_interface;

JavaVM *vm_jni_get_current_java_vm(void)
{
	return (JavaVM *) vm_jni_default_invoke_interface_ptr;
}

static jint JNI_GetVersion(JNIEnv *env)
{
	return JNI_VERSION;
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

	JNI_NOT_IMPLEMENTED;

	return 0;
}

static jobjectRefType JNI_GetObjectRefType(JNIEnv* env, jobject obj)
{
	JNI_NOT_IMPLEMENTED;
	return 0;
}

const struct JNINativeInterface_ defaultJNIEnv = {
	DEFINE_JNI_FUNCTION(GetVersion),
	DEFINE_JNI_FUNCTION(GetVersion),
	DEFINE_JNI_FUNCTION(DefineClass),
	DEFINE_JNI_FUNCTION(FindClass),
	DEFINE_JNI_FUNCTION(FromReflectedMethod),
	DEFINE_JNI_FUNCTION(FromReflectedField),
	DEFINE_JNI_FUNCTION(ToReflectedMethod),
	DEFINE_JNI_FUNCTION(GetSuperclass),
	DEFINE_JNI_FUNCTION(IsAssignableFrom),
	DEFINE_JNI_FUNCTION(ToReflectedField),
	DEFINE_JNI_FUNCTION(Throw),
	DEFINE_JNI_FUNCTION(ThrowNew),
	DEFINE_JNI_FUNCTION(ExceptionOccurred),
	DEFINE_JNI_FUNCTION(ExceptionDescribe),
	DEFINE_JNI_FUNCTION(ExceptionClear),
	DEFINE_JNI_FUNCTION(FatalError),
	DEFINE_JNI_FUNCTION(PushLocalFrame),
	DEFINE_JNI_FUNCTION(PopLocalFrame),
	DEFINE_JNI_FUNCTION(NewGlobalRef),
	DEFINE_JNI_FUNCTION(DeleteGlobalRef),
	DEFINE_JNI_FUNCTION(DeleteLocalRef),
	DEFINE_JNI_FUNCTION(IsSameObject),
	DEFINE_JNI_FUNCTION(NewLocalRef),
	DEFINE_JNI_FUNCTION(EnsureLocalCapacity),
	DEFINE_JNI_FUNCTION(AllocObject),
	DEFINE_JNI_FUNCTION(NewObject),
	DEFINE_JNI_FUNCTION(NewObjectV),
	DEFINE_JNI_FUNCTION(NewObjectA),
	DEFINE_JNI_FUNCTION(GetObjectClass),
	DEFINE_JNI_FUNCTION(IsInstanceOf),
	DEFINE_JNI_FUNCTION(GetMethodID),
	DEFINE_JNI_FUNCTION(CallObjectMethod),
	DEFINE_JNI_FUNCTION(CallObjectMethodV),
	DEFINE_JNI_FUNCTION(CallObjectMethodA),
	DEFINE_JNI_FUNCTION(CallBooleanMethod),
	DEFINE_JNI_FUNCTION(CallBooleanMethodV),
	DEFINE_JNI_FUNCTION(CallBooleanMethodA),
	DEFINE_JNI_FUNCTION(CallByteMethod),
	DEFINE_JNI_FUNCTION(CallByteMethodV),
	DEFINE_JNI_FUNCTION(CallByteMethodA),
	DEFINE_JNI_FUNCTION(CallCharMethod),
	DEFINE_JNI_FUNCTION(CallCharMethodV),
	DEFINE_JNI_FUNCTION(CallCharMethodA),
	DEFINE_JNI_FUNCTION(CallShortMethod),
	DEFINE_JNI_FUNCTION(CallShortMethodV),
	DEFINE_JNI_FUNCTION(CallShortMethodA),
	DEFINE_JNI_FUNCTION(CallIntMethod),
	DEFINE_JNI_FUNCTION(CallIntMethodV),
	DEFINE_JNI_FUNCTION(CallIntMethodA),
	DEFINE_JNI_FUNCTION(CallLongMethod),
	DEFINE_JNI_FUNCTION(CallLongMethodV),
	DEFINE_JNI_FUNCTION(CallLongMethodA),
	DEFINE_JNI_FUNCTION(CallFloatMethod),
	DEFINE_JNI_FUNCTION(CallFloatMethodV),
	DEFINE_JNI_FUNCTION(CallFloatMethodA),
	DEFINE_JNI_FUNCTION(CallDoubleMethod),
	DEFINE_JNI_FUNCTION(CallDoubleMethodV),
	DEFINE_JNI_FUNCTION(CallDoubleMethodA),
	DEFINE_JNI_FUNCTION(CallVoidMethod),
	DEFINE_JNI_FUNCTION(CallVoidMethodV),
	DEFINE_JNI_FUNCTION(CallVoidMethodA),
	DEFINE_JNI_FUNCTION(CallNonvirtualObjectMethod),
	DEFINE_JNI_FUNCTION(CallNonvirtualObjectMethodV),
	DEFINE_JNI_FUNCTION(CallNonvirtualObjectMethodA),
	DEFINE_JNI_FUNCTION(CallNonvirtualBooleanMethod),
	DEFINE_JNI_FUNCTION(CallNonvirtualBooleanMethodV),
	DEFINE_JNI_FUNCTION(CallNonvirtualBooleanMethodA),
	DEFINE_JNI_FUNCTION(CallNonvirtualByteMethod),
	DEFINE_JNI_FUNCTION(CallNonvirtualByteMethodV),
	DEFINE_JNI_FUNCTION(CallNonvirtualByteMethodA),
	DEFINE_JNI_FUNCTION(CallNonvirtualCharMethod),
	DEFINE_JNI_FUNCTION(CallNonvirtualCharMethodV),
	DEFINE_JNI_FUNCTION(CallNonvirtualCharMethodA),
	DEFINE_JNI_FUNCTION(CallNonvirtualShortMethod),
	DEFINE_JNI_FUNCTION(CallNonvirtualShortMethodV),
	DEFINE_JNI_FUNCTION(CallNonvirtualShortMethodA),
	DEFINE_JNI_FUNCTION(CallNonvirtualIntMethod),
	DEFINE_JNI_FUNCTION(CallNonvirtualIntMethodV),
	DEFINE_JNI_FUNCTION(CallNonvirtualIntMethodA),
	DEFINE_JNI_FUNCTION(CallNonvirtualLongMethod),
	DEFINE_JNI_FUNCTION(CallNonvirtualLongMethodV),
	DEFINE_JNI_FUNCTION(CallNonvirtualLongMethodA),
	DEFINE_JNI_FUNCTION(CallNonvirtualFloatMethod),
	DEFINE_JNI_FUNCTION(CallNonvirtualFloatMethodV),
	DEFINE_JNI_FUNCTION(CallNonvirtualFloatMethodA),
	DEFINE_JNI_FUNCTION(CallNonvirtualDoubleMethod),
	DEFINE_JNI_FUNCTION(CallNonvirtualDoubleMethodV),
	DEFINE_JNI_FUNCTION(CallNonvirtualDoubleMethodA),
	DEFINE_JNI_FUNCTION(CallNonvirtualVoidMethod),
	DEFINE_JNI_FUNCTION(CallNonvirtualVoidMethodV),
	DEFINE_JNI_FUNCTION(CallNonvirtualVoidMethodA),
	DEFINE_JNI_FUNCTION(GetFieldID),
	DEFINE_JNI_FUNCTION(GetObjectField),
	DEFINE_JNI_FUNCTION(GetBooleanField),
	DEFINE_JNI_FUNCTION(GetByteField),
	DEFINE_JNI_FUNCTION(GetCharField),
	DEFINE_JNI_FUNCTION(GetShortField),
	DEFINE_JNI_FUNCTION(GetIntField),
	DEFINE_JNI_FUNCTION(GetLongField),
	DEFINE_JNI_FUNCTION(GetFloatField),
	DEFINE_JNI_FUNCTION(GetDoubleField),
	DEFINE_JNI_FUNCTION(SetObjectField),
	DEFINE_JNI_FUNCTION(SetBooleanField),
	DEFINE_JNI_FUNCTION(SetByteField),
	DEFINE_JNI_FUNCTION(SetCharField),
	DEFINE_JNI_FUNCTION(SetShortField),
	DEFINE_JNI_FUNCTION(SetIntField),
	DEFINE_JNI_FUNCTION(SetLongField),
	DEFINE_JNI_FUNCTION(SetFloatField),
	DEFINE_JNI_FUNCTION(SetDoubleField),
	DEFINE_JNI_FUNCTION(GetStaticMethodID),
	DEFINE_JNI_FUNCTION(CallStaticObjectMethod),
	DEFINE_JNI_FUNCTION(CallStaticObjectMethodV),
	DEFINE_JNI_FUNCTION(CallStaticObjectMethodA),
	DEFINE_JNI_FUNCTION(CallStaticBooleanMethod),
	DEFINE_JNI_FUNCTION(CallStaticBooleanMethodV),
	DEFINE_JNI_FUNCTION(CallStaticBooleanMethodA),
	DEFINE_JNI_FUNCTION(CallStaticByteMethod),
	DEFINE_JNI_FUNCTION(CallStaticByteMethodV),
	DEFINE_JNI_FUNCTION(CallStaticByteMethodA),
	DEFINE_JNI_FUNCTION(CallStaticCharMethod),
	DEFINE_JNI_FUNCTION(CallStaticCharMethodV),
	DEFINE_JNI_FUNCTION(CallStaticCharMethodA),
	DEFINE_JNI_FUNCTION(CallStaticShortMethod),
	DEFINE_JNI_FUNCTION(CallStaticShortMethodV),
	DEFINE_JNI_FUNCTION(CallStaticShortMethodA),
	DEFINE_JNI_FUNCTION(CallStaticIntMethod),
	DEFINE_JNI_FUNCTION(CallStaticIntMethodV),
	DEFINE_JNI_FUNCTION(CallStaticIntMethodA),
	DEFINE_JNI_FUNCTION(CallStaticLongMethod),
	DEFINE_JNI_FUNCTION(CallStaticLongMethodV),
	DEFINE_JNI_FUNCTION(CallStaticLongMethodA),
	DEFINE_JNI_FUNCTION(CallStaticFloatMethod),
	DEFINE_JNI_FUNCTION(CallStaticFloatMethodV),
	DEFINE_JNI_FUNCTION(CallStaticFloatMethodA),
	DEFINE_JNI_FUNCTION(CallStaticDoubleMethod),
	DEFINE_JNI_FUNCTION(CallStaticDoubleMethodV),
	DEFINE_JNI_FUNCTION(CallStaticDoubleMethodA),
	DEFINE_JNI_FUNCTION(CallStaticVoidMethod),
	DEFINE_JNI_FUNCTION(CallStaticVoidMethodV),
	DEFINE_JNI_FUNCTION(CallStaticVoidMethodA),
	DEFINE_JNI_FUNCTION(GetStaticFieldID),
	DEFINE_JNI_FUNCTION(GetStaticObjectField),
	DEFINE_JNI_FUNCTION(GetStaticBooleanField),
	DEFINE_JNI_FUNCTION(GetStaticByteField),
	DEFINE_JNI_FUNCTION(GetStaticCharField),
	DEFINE_JNI_FUNCTION(GetStaticShortField),
	DEFINE_JNI_FUNCTION(GetStaticIntField),
	DEFINE_JNI_FUNCTION(GetStaticLongField),
	DEFINE_JNI_FUNCTION(GetStaticFloatField),
	DEFINE_JNI_FUNCTION(GetStaticDoubleField),
	DEFINE_JNI_FUNCTION(SetStaticObjectField),
	DEFINE_JNI_FUNCTION(SetStaticBooleanField),
	DEFINE_JNI_FUNCTION(SetStaticByteField),
	DEFINE_JNI_FUNCTION(SetStaticCharField),
	DEFINE_JNI_FUNCTION(SetStaticShortField),
	DEFINE_JNI_FUNCTION(SetStaticIntField),
	DEFINE_JNI_FUNCTION(SetStaticLongField),
	DEFINE_JNI_FUNCTION(SetStaticFloatField),
	DEFINE_JNI_FUNCTION(SetStaticDoubleField),
	DEFINE_JNI_FUNCTION(NewString),
	DEFINE_JNI_FUNCTION(GetStringLength),
	DEFINE_JNI_FUNCTION(GetStringChars),
	DEFINE_JNI_FUNCTION(ReleaseStringChars),
	DEFINE_JNI_FUNCTION(NewStringUTF),
	DEFINE_JNI_FUNCTION(GetStringUTFLength),
	DEFINE_JNI_FUNCTION(GetStringUTFChars),
	DEFINE_JNI_FUNCTION(ReleaseStringUTFChars),
	DEFINE_JNI_FUNCTION(GetArrayLength),
	DEFINE_JNI_FUNCTION(NewObjectArray),
	DEFINE_JNI_FUNCTION(GetObjectArrayElement),
	DEFINE_JNI_FUNCTION(SetObjectArrayElement),
	DEFINE_JNI_FUNCTION(NewBooleanArray),
	DEFINE_JNI_FUNCTION(NewByteArray),
	DEFINE_JNI_FUNCTION(NewCharArray),
	DEFINE_JNI_FUNCTION(NewShortArray),
	DEFINE_JNI_FUNCTION(NewIntArray),
	DEFINE_JNI_FUNCTION(NewLongArray),
	DEFINE_JNI_FUNCTION(NewFloatArray),
	DEFINE_JNI_FUNCTION(NewDoubleArray),
	DEFINE_JNI_FUNCTION(GetBooleanArrayElements),
	DEFINE_JNI_FUNCTION(GetByteArrayElements),
	DEFINE_JNI_FUNCTION(GetCharArrayElements),
	DEFINE_JNI_FUNCTION(GetShortArrayElements),
	DEFINE_JNI_FUNCTION(GetIntArrayElements),
	DEFINE_JNI_FUNCTION(GetLongArrayElements),
	DEFINE_JNI_FUNCTION(GetFloatArrayElements),
	DEFINE_JNI_FUNCTION(GetDoubleArrayElements),
	DEFINE_JNI_FUNCTION(ReleaseBooleanArrayElements),
	DEFINE_JNI_FUNCTION(ReleaseByteArrayElements),
	DEFINE_JNI_FUNCTION(ReleaseCharArrayElements),
	DEFINE_JNI_FUNCTION(ReleaseShortArrayElements),
	DEFINE_JNI_FUNCTION(ReleaseIntArrayElements),
	DEFINE_JNI_FUNCTION(ReleaseLongArrayElements),
	DEFINE_JNI_FUNCTION(ReleaseFloatArrayElements),
	DEFINE_JNI_FUNCTION(ReleaseDoubleArrayElements),
	DEFINE_JNI_FUNCTION(GetBooleanArrayRegion),
	DEFINE_JNI_FUNCTION(GetByteArrayRegion),
	DEFINE_JNI_FUNCTION(GetCharArrayRegion),
	DEFINE_JNI_FUNCTION(GetShortArrayRegion),
	DEFINE_JNI_FUNCTION(GetIntArrayRegion),
	DEFINE_JNI_FUNCTION(GetLongArrayRegion),
	DEFINE_JNI_FUNCTION(GetFloatArrayRegion),
	DEFINE_JNI_FUNCTION(GetDoubleArrayRegion),
	DEFINE_JNI_FUNCTION(SetBooleanArrayRegion),
	DEFINE_JNI_FUNCTION(SetByteArrayRegion),
	DEFINE_JNI_FUNCTION(SetCharArrayRegion),
	DEFINE_JNI_FUNCTION(SetShortArrayRegion),
	DEFINE_JNI_FUNCTION(SetIntArrayRegion),
	DEFINE_JNI_FUNCTION(SetLongArrayRegion),
	DEFINE_JNI_FUNCTION(SetFloatArrayRegion),
	DEFINE_JNI_FUNCTION(SetDoubleArrayRegion),
	DEFINE_JNI_FUNCTION(RegisterNatives),
	DEFINE_JNI_FUNCTION(UnregisterNatives),
	DEFINE_JNI_FUNCTION(MonitorEnter),
	DEFINE_JNI_FUNCTION(MonitorExit),
	DEFINE_JNI_FUNCTION(GetJavaVM),
	DEFINE_JNI_FUNCTION(GetStringRegion),
	DEFINE_JNI_FUNCTION(GetStringUTFRegion),
	DEFINE_JNI_FUNCTION(GetPrimitiveArrayCritical),
	DEFINE_JNI_FUNCTION(ReleasePrimitiveArrayCritical),
	DEFINE_JNI_FUNCTION(GetStringCritical),
	DEFINE_JNI_FUNCTION(ReleaseStringCritical),
	DEFINE_JNI_FUNCTION(NewWeakGlobalRef),
	DEFINE_JNI_FUNCTION(DeleteWeakGlobalRef),
	DEFINE_JNI_FUNCTION(ExceptionCheck),
	DEFINE_JNI_FUNCTION(NewDirectByteBuffer),
	DEFINE_JNI_FUNCTION(GetDirectBufferAddress),
	DEFINE_JNI_FUNCTION(GetDirectBufferCapacity),
	DEFINE_JNI_FUNCTION(GetObjectRefType),
};

const struct JNINativeInterface_* vm_jni_default_env = &defaultJNIEnv;
const struct JNINativeInterface_** vm_jni_default_env_ptr = &vm_jni_default_env;

JNIEnv *vm_jni_get_jni_env(void)
{
	return (JNIEnv *) vm_jni_default_env_ptr;
}
