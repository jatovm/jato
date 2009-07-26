#include "vm/class.h"
#include "vm/preload.h"

struct vm_class *vm_java_lang_Object;
struct vm_class *vm_java_lang_Class;
struct vm_field *vm_java_lang_Class_vmdata;
struct vm_class *vm_java_lang_Cloneable;
struct vm_class *vm_java_lang_String;

struct vm_field *vm_java_lang_String_offset;
struct vm_field *vm_java_lang_String_count;
struct vm_field *vm_java_lang_String_value;
struct vm_field *vm_java_lang_Throwable_detailMessage;
struct vm_field *vm_java_lang_VMThrowable_vmdata;

struct vm_class *vm_java_lang_Throwable;
struct vm_class *vm_java_lang_VMThrowable;
struct vm_class *vm_java_lang_StackTraceElement;
struct vm_class *vm_array_of_java_lang_StackTraceElement;
struct vm_class *vm_java_lang_Error;
struct vm_class *vm_java_lang_ArithmeticException;
struct vm_class *vm_java_lang_NullPointerException;
struct vm_class *vm_java_lang_NoClassDefFoundError;
struct vm_class *vm_java_lang_UnsatisfiedLinkError;
struct vm_class *vm_java_lang_ArrayIndexOutOfBoundsException;
struct vm_class *vm_java_lang_ArrayStoreException;
struct vm_class *vm_java_lang_RuntimeException;
struct vm_class *vm_java_lang_ExceptionInInitializerError;
struct vm_class *vm_java_lang_NegativeArraySizeException;
struct vm_class *vm_java_lang_ClassCastException;
struct vm_class *vm_java_lang_NoSuchFieldError;
struct vm_class *vm_java_lang_NoSuchMethodError;
struct vm_class *vm_java_lang_StackOverflowError;

struct vm_method *vm_java_lang_Throwable_initCause;
struct vm_method *vm_java_lang_Throwable_getCause;
struct vm_method *vm_java_lang_Throwable_stackTraceString;
struct vm_method *vm_java_lang_Throwable_getStackTrace;
struct vm_method *vm_java_lang_Throwable_setStackTrace;
struct vm_method *vm_java_lang_StackTraceElement_init;
