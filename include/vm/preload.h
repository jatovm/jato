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

#define PRELOAD_METHOD(class, method_name, method_type, var_name) \
	extern struct vm_method *var_name;
#include "vm/preload-methods.h"
#undef PRELOAD_METHOD

extern bool preload_finished;

int vm_preload_add_class_fixup(struct vm_class *vmc);
int preload_vm_classes(void);

#endif
