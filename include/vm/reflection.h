#ifndef __JATO_VM_REFLECTION_H
#define __JATO_VM_REFLECTION_H

#include "vm/jni.h"

struct vm_object;

struct vm_class *vm_object_to_vm_class(struct vm_object *object);
struct vm_field *vm_object_to_vm_field(struct vm_object *field);
struct vm_object *vm_method_to_java_lang_reflect_method(struct vm_method *vmm, jobject clazz, int method_index);
struct vm_object *vm_field_to_java_lang_reflect_field(struct vm_field *vmf, jobject clazz, int field_index);

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
struct vm_object *java_lang_reflect_VMField_get(struct vm_object *this, struct vm_object *o);

jlong java_lang_reflect_VMField_getLong(struct vm_object *this, struct vm_object *o);
jint java_lang_reflect_VMField_getInt(struct vm_object *this, struct vm_object *o);
jshort java_lang_reflect_VMField_getShort(struct vm_object *this, struct vm_object *o);
jdouble java_lang_reflect_VMField_getDouble(struct vm_object *this, struct vm_object *o);
jfloat java_lang_reflect_VMField_getFloat(struct vm_object *this, struct vm_object *o);
jbyte java_lang_reflect_VMField_getByte(struct vm_object *this, struct vm_object *o);
jchar java_lang_reflect_VMField_getChar(struct vm_object *this, struct vm_object *o);
jboolean java_lang_reflect_VMField_getBoolean(struct vm_object *this, struct vm_object *o);
jobject java_lang_reflect_VMMethod_getParameterAnnotations(jobject klass);
jint java_lang_reflect_VMField_getModifiersInternal(struct vm_object *this);
struct vm_object *java_lang_reflect_VMField_getType(struct vm_object *this);

struct vm_method *vm_object_to_vm_method(struct vm_object *method);
jint native_method_get_modifiers_internal(struct vm_object *this);
struct vm_object *
native_method_invokenative(struct vm_object *method, struct vm_object *o,
			   struct vm_object *args,
			   struct vm_object *declaringClass,
			   jint slot);
struct vm_object *native_vmmethod_invoke(struct vm_object *vm_method, struct vm_object *o, struct vm_object *args);
void java_lang_reflect_VMField_set(struct vm_object *this, struct vm_object *o, struct vm_object *value_obj);
struct vm_object *native_method_getreturntype(struct vm_object *method);
struct vm_object *native_method_get_exception_types(struct vm_object *method);
struct vm_object *native_method_get_default_value(struct vm_object *method);
jobject native_vmarray_createobjectarray(jobject type, int dim);

#endif /* __JATO_VM_REFLECTION_H */
