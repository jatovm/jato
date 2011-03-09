#ifndef JATO__VM_BOXING_H
#define JATO__VM_BOXING_H

#include "vm/types.h"
#include "vm/jni.h"

struct vm_object;

struct vm_object *enum_to_object(struct vm_object *enum_type, struct vm_object *name);
struct vm_object *boolean_to_object(jboolean value);
struct vm_object *byte_to_object(jbyte value);
struct vm_object *char_to_object(jchar value);
struct vm_object *short_to_object(jshort value);
struct vm_object *long_to_object(jlong value);
struct vm_object *int_to_object(jint value);
struct vm_object *float_to_object(jfloat value);
struct vm_object *double_to_object(jdouble value);
struct vm_object *jvalue_to_object(union jvalue *value, enum vm_type vm_type);

#endif /* JATO__VM_BOXING_H */
