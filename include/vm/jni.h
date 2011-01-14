#ifndef JATO_VM_JNI_H
#define JATO_VM_JNI_H

#include <stdbool.h>
#include <stdint.h>

/* Version numbers.  */
#define JNI_VERSION_1_1 0x00010001
#define JNI_VERSION_1_2 0x00010002
#define JNI_VERSION_1_4 0x00010004
#define JNI_VERSION_1_6 0x00010006

/* Used when releasing array elements.  */
#define JNI_COMMIT 1
#define JNI_ABORT  2

/* Error codes */
#define JNI_OK            0
#define JNI_ERR          (-1)
#define JNI_EDETACHED    (-2)
#define JNI_EVERSION     (-3)

#define JNI_FALSE  0
#define JNI_TRUE   1

#define JNIEXPORT
#define JNICALL

struct vm_object;
struct vm_class;
struct vm_method;

typedef uint8_t jboolean;
typedef int8_t jbyte;
typedef uint16_t jchar;
typedef int16_t jshort;
typedef int32_t jint;
typedef int64_t jlong;
typedef uint32_t jfloat;
typedef uint64_t jdouble;

typedef jint jsize;

typedef struct vm_object *jobject;

typedef jobject jstring;
typedef jobject jarray;
typedef jobject jclass;
typedef jobject jobjectArray;
typedef jobject jthrowable;

typedef struct vm_field *jfieldID;
typedef struct vm_method *jmethodID;

typedef jobject jweak;

union jvalue {
	jboolean z;
	jbyte    b;
	jchar    c;
	jshort   s;
	jint     i;
	jlong    j;
	jfloat   f;
	jdouble  d;
	jobject  l;
};

typedef struct vm_jni_env JNIEnv;

struct vm_jni_env {
	void **jni_table;
};

struct java_vm {
	void **jni_invoke_interface_table;
};

typedef struct java_vm JavaVM;

void vm_jni_init(void);
void vm_jni_init_interface(void);
struct vm_jni_env *vm_jni_get_jni_env(void);
struct java_vm *vm_jni_get_current_java_vm(void);
int vm_jni_load_object(const char *name, struct vm_object *classloader);
void *vm_jni_lookup_method(const char *class_name, const char *method_name,
			   const char *method_type);
bool vm_jni_check_trap(void *ptr);

#endif
