#ifndef _VM_JAVA_LANG_H
#define _VM_JAVA_LANG_H

extern struct vm_class *vm_java_lang_Object;
extern struct vm_class *vm_java_lang_Class;
extern struct vm_class *vm_java_lang_Cloneable;
extern struct vm_class *vm_java_lang_String;
extern struct vm_class *vm_java_lang_Throwable;
extern struct vm_class *vm_java_util_Properties;
extern struct vm_class *vm_java_lang_VMThrowable;
extern struct vm_class *vm_java_lang_StackTraceElement;
extern struct vm_class *vm_array_of_java_lang_StackTraceElement;
extern struct vm_class *vm_java_lang_Error;
extern struct vm_class *vm_java_lang_ArithmeticException;
extern struct vm_class *vm_java_lang_NullPointerException;
extern struct vm_class *vm_java_lang_NegativeArraySizeException;
extern struct vm_class *vm_java_lang_ClassCastException;
extern struct vm_class *vm_java_lang_NoClassDefFoundError;
extern struct vm_class *vm_java_lang_UnsatisfiedLinkError;
extern struct vm_class *vm_java_lang_ArrayIndexOutOfBoundsException;
extern struct vm_class *vm_java_lang_ArrayStoreException;
extern struct vm_class *vm_java_lang_RuntimeException;
extern struct vm_class *vm_java_lang_ExceptionInInitializerError;
extern struct vm_class *vm_java_lang_NoSuchFieldError;
extern struct vm_class *vm_java_lang_NoSuchMethodError;
extern struct vm_class *vm_java_lang_StackOverflowError;
extern struct vm_class *vm_boolean_class;
extern struct vm_class *vm_char_class;
extern struct vm_class *vm_float_class;
extern struct vm_class *vm_double_class;
extern struct vm_class *vm_byte_class;
extern struct vm_class *vm_short_class;
extern struct vm_class *vm_int_class;
extern struct vm_class *vm_long_class;

extern struct vm_field *vm_java_lang_Class_vmdata;
extern struct vm_field *vm_java_lang_String_offset;
extern struct vm_field *vm_java_lang_String_count;
extern struct vm_field *vm_java_lang_String_value;
extern struct vm_field *vm_java_lang_Throwable_detailMessage;
extern struct vm_field *vm_java_lang_VMThrowable_vmdata;

extern struct vm_method *vm_java_util_Properties_setProperty;
extern struct vm_method *vm_java_lang_Throwable_initCause;
extern struct vm_method *vm_java_lang_Throwable_getCause;
extern struct vm_method *vm_java_lang_Throwable_stackTraceString;
extern struct vm_method *vm_java_lang_Throwable_getStackTrace;
extern struct vm_method *vm_java_lang_Throwable_setStackTrace;
extern struct vm_method *vm_java_lang_StackTraceElement_init;

int preload_vm_classes(void);

#endif
