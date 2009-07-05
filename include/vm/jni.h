#ifndef JATO_VM_JNI_H
#define JATO_VM_JNI_H

#include <stdbool.h>
#include <stdint.h>

#define JNI_FALSE  0
#define JNI_TRUE   1

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
typedef jobject jclass;
typedef struct vm_field *jfieldID;
typedef struct vm_method *jmethodID;

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

int vm_jni_nr_loaded_objects;
void **vm_jni_loaded_objects;

struct vm_jni_env {
	void **jni_table;
};

void vm_jni_init(void);
struct vm_jni_env *vm_jni_get_jni_env();
int vm_jni_load_object(const char *name);
void *vm_jni_lookup_method(const char *class_name, const char *method_name,
			   const char *method_type);
bool vm_jni_check_trap(void *ptr);

#endif
