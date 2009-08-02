#include "vm/call.h"
#include "vm/class.h"
#include "vm/die.h"
#include "vm/preload.h"
#include "vm/reflection.h"

struct vm_object *
native_vmclass_get_declared_constructors(struct vm_object *class_object,
					 jboolean public_only)
{
	struct vm_class *vmc;

	if (!vm_object_is_instance_of(class_object, vm_java_lang_Class))
		return NULL;

	vmc = vm_class_get_class_from_class_object(class_object);

	int count = 0;

	for (int i = 0; i < vmc->class->methods_count; i++) {
		struct vm_method *vmm = &vmc->methods[i];

		if (vm_method_is_constructor(vmm))
			count++;
	}

	struct vm_object *array;

	array = vm_object_alloc_array(vm_array_of_java_lang_reflect_Constructor,
				      count);
	if (!array) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	int index = 0;

	for (int i = 0; i < vmc->class->methods_count; i++) {
		struct vm_method *vmm = &vmc->methods[i];

		if (!vm_method_is_constructor(vmm))
			continue;

		struct vm_object *ctor
			= vm_object_alloc(vm_java_lang_reflect_Constructor);

		if (!ctor) {
			NOT_IMPLEMENTED;
			return NULL;
		}

		field_set_object(ctor, vm_java_lang_reflect_Constructor_clazz,
				 class_object);
		field_set_int32(ctor, vm_java_lang_reflect_Constructor_slot,
				i);

		array_set_field_ptr(array, index++, ctor);
	}

	return array;
}

struct vm_object *
native_constructor_get_parameter_types(struct vm_object *ctor)
{
	struct vm_object *array;
	struct vm_object *clazz;
	struct vm_class *class;
	struct vm_method *vmm;
	int slot;

	clazz = field_get_object(ctor, vm_java_lang_reflect_Constructor_clazz);
	slot = field_get_int32(ctor, vm_java_lang_reflect_Constructor_slot);

	class = vm_class_get_class_from_class_object(clazz);
	vmm = &class->methods[slot];

	/* TODO: We support only ()V constructors yet. */
	assert(!strcmp(vmm->type, "()V"));

	array = vm_object_alloc_array(vm_array_of_java_lang_Class, 0);
	return array;
}

jint native_constructor_get_modifiers_internal(struct vm_object *ctor)
{
	struct vm_object *clazz;
	struct vm_class *class;
	struct vm_method *vmm;
	int slot;

	clazz = field_get_object(ctor, vm_java_lang_reflect_Constructor_clazz);
	slot = field_get_int32(ctor, vm_java_lang_reflect_Constructor_slot);

	class = vm_class_get_class_from_class_object(clazz);
	vmm = &class->methods[slot];

	return vmm->method->access_flags;
}

struct vm_object *
native_constructor_construct_native(struct vm_object *this,
				    struct vm_object *args,
				    struct vm_object *declaring_class,
				    int slot)
{
	struct vm_object *result;
	struct vm_class *class;
	struct vm_method *vmm;

	if (!vm_object_is_instance_of(declaring_class, vm_java_lang_Class))
		return NULL;

	class = vm_class_get_class_from_class_object(declaring_class);
	vmm = &class->methods[slot];

	result = vm_object_alloc(class);

	NOT_IMPLEMENTED;

	/* TODO: We support only ()V constructors yet. */
	assert(args == NULL || args->array_length == 0);

	vm_call_method_object(vmm, result);
	return result;
}
