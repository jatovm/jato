#ifndef __JATO_VM_REFLECTION_H
#define __JATO_VM_REFLECTION_H

#include "vm/jni.h"

struct vm_object;

struct vm_class *vm_object_to_vm_class(struct vm_object *object);
struct vm_field *vm_object_to_vm_field(struct vm_object *field);
struct vm_object *vm_method_to_java_lang_reflect_method(struct vm_method *vmm, jobject clazz, int method_index);

struct vm_object *
native_constructor_get_parameter_types(struct vm_object *ctor);
struct vm_object *
native_method_get_parameter_types(struct vm_object *ctor);
jint native_constructor_get_modifiers_internal(struct vm_object *ctor);
struct vm_object *
native_constructor_construct_native(struct vm_object *this,
				    struct vm_object *args,
				    struct vm_object *declaring_class,
				    int slot);
struct vm_object *native_vmconstructor_construct(struct vm_object *this, struct vm_object *args);
struct vm_object *native_vmconstructor_get_exception_types(struct vm_object *method);
struct vm_object *native_field_get(struct vm_object *this, struct vm_object *o);

jlong native_field_get_long(struct vm_object *this, struct vm_object *o);
jint native_field_get_int(struct vm_object *this, struct vm_object *o);
jshort native_field_get_short(struct vm_object *this, struct vm_object *o);
jdouble native_field_get_double(struct vm_object *this, struct vm_object *o);
jfloat native_field_get_float(struct vm_object *this, struct vm_object *o);
jbyte native_field_get_byte(struct vm_object *this, struct vm_object *o);
jchar native_field_get_char(struct vm_object *this, struct vm_object *o);

jint native_field_get_modifiers_internal(struct vm_object *this);
struct vm_object *native_field_gettype(struct vm_object *this);

struct vm_method *vm_object_to_vm_method(struct vm_object *method);
jint native_method_get_modifiers_internal(struct vm_object *this);
struct vm_object *
native_method_invokenative(struct vm_object *method, struct vm_object *o,
			   struct vm_object *args,
			   struct vm_object *declaringClass,
			   jint slot);
struct vm_object *native_vmmethod_invoke(struct vm_object *vm_method, struct vm_object *o, struct vm_object *args);
void native_field_set(struct vm_object *this, struct vm_object *o, struct vm_object *value_obj);
struct vm_object *native_method_getreturntype(struct vm_object *method);
struct vm_object *native_method_get_exception_types(struct vm_object *method);
struct vm_object *native_method_get_default_value(struct vm_object *method);

jobject native_vmarray_createobjectarray(jobject type, int dim);

#endif /* __JATO_VM_REFLECTION_H */
