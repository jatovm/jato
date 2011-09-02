#ifndef _VM_JAVA_LANG_H
#define _VM_JAVA_LANG_H

#include <stdbool.h>

#ifdef CONFIG_32_BIT
#define GNU_CLASSPATH_PATH_POINTER_NN	"gnu/classpath/Pointer32"
#else
#define GNU_CLASSPATH_PATH_POINTER_NN	"gnu/classpath/Pointer64"
#endif

#define PRELOAD_CLASS(class_name, var_name, opt) \
	extern struct vm_class *var_name;
#include "vm/preload-classes.h"
#undef PRELOAD_CLASS

extern struct vm_class *vm_array_of_boolean;
extern struct vm_class *vm_array_of_byte;
extern struct vm_class *vm_array_of_char;
extern struct vm_class *vm_array_of_double;
extern struct vm_class *vm_array_of_float;
extern struct vm_class *vm_array_of_int;
extern struct vm_class *vm_array_of_long;
extern struct vm_class *vm_array_of_short;

extern struct vm_class *vm_boolean_class;
extern struct vm_class *vm_char_class;
extern struct vm_class *vm_float_class;
extern struct vm_class *vm_double_class;
extern struct vm_class *vm_byte_class;
extern struct vm_class *vm_short_class;
extern struct vm_class *vm_int_class;
extern struct vm_class *vm_long_class;
extern struct vm_class *vm_void_class;

extern struct vm_field *vm_java_lang_Class_vmdata;
extern struct vm_field *vm_java_lang_String_offset;
extern struct vm_field *vm_java_lang_String_count;
extern struct vm_field *vm_java_lang_String_value;
extern struct vm_field *vm_java_lang_Throwable_detailMessage;
extern struct vm_field *vm_java_lang_VMThrowable_vmdata;
extern struct vm_field *vm_java_lang_Thread_daemon;
extern struct vm_field *vm_java_lang_Thread_group;
extern struct vm_field *vm_java_lang_Thread_name;
extern struct vm_field *vm_java_lang_Thread_priority;
extern struct vm_field *vm_java_lang_Thread_contextClassLoader;
extern struct vm_field *vm_java_lang_Thread_contextClassLoaderIsSystemClassLoader;
extern struct vm_field *vm_java_lang_Thread_vmThread;
extern struct vm_field *vm_java_lang_VMThread_thread;
extern struct vm_field *vm_java_lang_VMThread_vmdata;
extern struct vm_field *vm_java_lang_reflect_Constructor_clazz;
extern struct vm_field *vm_java_lang_reflect_Constructor_cons;
extern struct vm_field *vm_java_lang_reflect_Constructor_slot;
extern struct vm_field *vm_java_lang_reflect_Field_declaringClass;
extern struct vm_field *vm_java_lang_reflect_Field_f;
extern struct vm_field *vm_java_lang_reflect_Field_name;
extern struct vm_field *vm_java_lang_reflect_Field_slot;
extern struct vm_field *vm_java_lang_reflect_Method_declaringClass;
extern struct vm_field *vm_java_lang_reflect_Method_m;
extern struct vm_field *vm_java_lang_reflect_Method_name;
extern struct vm_field *vm_java_lang_reflect_Method_slot;
extern struct vm_field *vm_java_lang_reflect_VMConstructor_clazz;
extern struct vm_field *vm_java_lang_reflect_VMConstructor_slot;
extern struct vm_field *vm_java_lang_reflect_VMField_clazz;
extern struct vm_field *vm_java_lang_reflect_VMField_name;
extern struct vm_field *vm_java_lang_reflect_VMField_slot;
extern struct vm_field *vm_java_lang_reflect_VMMethod_clazz;
extern struct vm_field *vm_java_lang_reflect_VMMethod_name;
extern struct vm_field *vm_java_lang_reflect_VMMethod_slot;
extern struct vm_field *vm_java_lang_reflect_VMMethod_m;
extern struct vm_field *vm_java_lang_ClassLoader_systemClassLoader;
extern struct vm_field *vm_java_lang_ref_Reference_referent;
extern struct vm_field *vm_java_lang_ref_Reference_lock;
extern struct vm_field *vm_java_nio_Buffer_address;
extern struct vm_field *vm_gnu_classpath_PointerNN_data;

extern struct vm_method *vm_java_util_Properties_setProperty;
extern struct vm_method *vm_java_util_HashMap_init;
extern struct vm_method *vm_java_util_HashMap_put;
extern struct vm_method *vm_java_lang_Throwable_initCause;
extern struct vm_method *vm_java_lang_Throwable_getCause;
extern struct vm_method *vm_java_lang_Throwable_stackTraceString;
extern struct vm_method *vm_java_lang_Throwable_getStackTrace;
extern struct vm_method *vm_java_lang_Throwable_setStackTrace;
extern struct vm_method *vm_java_lang_StackTraceElement_init;
extern struct vm_method *vm_java_lang_Thread_init;
extern struct vm_method *vm_java_lang_Thread_isDaemon;
extern struct vm_method *vm_java_lang_Thread_getName;
extern struct vm_method *vm_java_lang_ThreadGroup_init;
extern struct vm_method *vm_java_lang_ThreadGroup_addThread;
extern struct vm_method *vm_java_lang_VMThread_init;
extern struct vm_method *vm_java_lang_VMThread_run;
extern struct vm_method *vm_java_lang_System_exit;
extern struct vm_method *vm_java_lang_Boolean_booleanValue;
extern struct vm_method *vm_java_lang_Boolean_init;
extern struct vm_method *vm_java_lang_Boolean_valueOf;
extern struct vm_method *vm_java_lang_Byte_init;
extern struct vm_method *vm_java_lang_Byte_valueOf;
extern struct vm_method *vm_java_lang_Character_charValue;
extern struct vm_method *vm_java_lang_Character_init;
extern struct vm_method *vm_java_lang_Character_valueOf;
extern struct vm_method *vm_java_lang_Class_init;
extern struct vm_method *vm_java_lang_Double_init;
extern struct vm_method *vm_java_lang_Double_valueOf;
extern struct vm_method *vm_java_lang_Enum_valueOf;
extern struct vm_method *vm_java_lang_Float_init;
extern struct vm_method *vm_java_lang_Float_valueOf;
extern struct vm_method *vm_java_lang_InheritableThreadLocal_newChildThread;
extern struct vm_method *vm_java_lang_Integer_init;
extern struct vm_method *vm_java_lang_Integer_valueOf;
extern struct vm_method *vm_java_lang_Long_init;
extern struct vm_method *vm_java_lang_Long_valueOf;
extern struct vm_method *vm_java_lang_Short_init;
extern struct vm_method *vm_java_lang_Short_valueOf;
extern struct vm_method *vm_java_lang_String_length;
extern struct vm_method *vm_java_lang_ClassLoader_loadClass;
extern struct vm_method *vm_java_lang_ClassLoader_getSystemClassLoader;
extern struct vm_method *vm_java_lang_Number_intValue;
extern struct vm_method *vm_java_lang_Number_floatValue;
extern struct vm_method *vm_java_lang_Number_longValue;
extern struct vm_method *vm_java_lang_Number_doubleValue;
extern struct vm_method *vm_java_lang_ref_Reference_clear;
extern struct vm_method *vm_java_lang_ref_Reference_enqueue;
extern struct vm_method *vm_java_nio_DirectByteBufferImpl_ReadWrite_init;
extern struct vm_method *vm_sun_reflect_annotation_AnnotationInvocationHandler_create;

extern bool preload_finished;

int vm_preload_add_class_fixup(struct vm_class *vmc);
int preload_vm_classes(void);

#endif
