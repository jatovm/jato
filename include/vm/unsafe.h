#ifndef VM_UNSAFE_H
#define VM_UNSAFE_H

#include "vm/jni.h"

struct vm_object;

jboolean native_unsafe_compare_and_swap_int(struct vm_object *this, struct vm_object *obj, jlong offset, jint expect, jint update);
jlong native_unsafe_object_field_offset(struct vm_object *this, struct vm_object *field);

#endif /* VM_UNSAFE_H */
