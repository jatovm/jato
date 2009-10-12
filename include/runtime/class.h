#ifndef RUNTIME_CLASS_H
#define RUNTIME_CLASS_H

#include "vm/jni.h"

struct vm_object;

struct vm_object *native_vmclass_getclassloader(struct vm_object *object);
struct vm_object *native_vmclass_forname(struct vm_object *name,
					 jboolean initialize,
					 struct vm_object *loader);
struct vm_object *native_vmclass_getname(struct vm_object *object);
int32_t native_vmclass_is_anonymous_class(struct vm_object *object);
jboolean native_vmclass_is_assignable_from(struct vm_object *clazz_1,
					   struct vm_object *clazz_2);
int32_t native_vmclass_isarray(struct vm_object *object);
int32_t native_vmclass_isprimitive(struct vm_object *object);
jint native_vmclass_getmodifiers(struct vm_object *clazz);
struct vm_object *native_vmclass_getcomponenttype(struct vm_object *object);
jboolean native_vmclass_isinstance(struct vm_object *clazz,
				   struct vm_object *object);
jboolean native_vmclass_isinterface(struct vm_object *clazz);

#endif /* RUNTIME_CLASS_H */
