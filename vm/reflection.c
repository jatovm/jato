#include "vm/call.h"
#include "vm/class.h"
#include "vm/classloader.h"
#include "vm/die.h"
#include "vm/preload.h"
#include "vm/reflection.h"
#include "vm/types.h"

#include "jit/args.h"
#include "jit/exception.h"

static struct vm_class *to_vmclass(struct vm_object *object)
{
	struct vm_class *vmc;

	if (!object)
		goto throw;

	vmc = vm_class_get_class_from_class_object(object);
	if (!vmc)
		goto throw;

	return vmc;
throw:
	signal_new_exception(vm_java_lang_NullPointerException, NULL);
	return NULL;
}

struct vm_object *
native_vmclass_get_declared_fields(struct vm_object *clazz,
				   jboolean public_only)
{
	struct vm_class *vmc;

	vmc = to_vmclass(clazz);
	if (!vmc)
		return NULL;

	int count;

	if (public_only) {
		count = 0;

		for (int i = 0; i < vmc->class->fields_count; i++) {
			struct vm_field *vmf = &vmc->fields[i];

			if (vm_field_is_public(vmf))
				count ++;
		}
	} else {
		count = vmc->class->fields_count;
	}

	struct vm_object *array
		= vm_object_alloc_array(vm_array_of_java_lang_reflect_Field,
					count);
	if (!array) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	int index = 0;

	for (int i = 0; i < vmc->class->fields_count; i++) {
		struct vm_field *vmf = &vmc->fields[i];

		if (public_only && !vm_field_is_public(vmf))
			continue;

		struct vm_object *field
			= vm_object_alloc(vm_java_lang_reflect_Field);

		if (!field) {
			NOT_IMPLEMENTED;
			return NULL;
		}

		struct vm_object *name_object
			= vm_object_alloc_string_from_c(vmf->name);

		if (!name_object) {
			NOT_IMPLEMENTED;
			return NULL;
		}

		field_set_object(field, vm_java_lang_reflect_Field_declaringClass,
				 clazz);
		field_set_object(field, vm_java_lang_reflect_Field_name,
				 name_object);
		field_set_int32(field, vm_java_lang_reflect_Field_slot,
				i);

		array_set_field_ptr(array, index++, field);
	}

	return array;
}

struct vm_object *
native_vmclass_get_declared_methods(struct vm_object *clazz,
				    jboolean public_only)
{
	struct vm_class *vmc;

	vmc = to_vmclass(clazz);
	if (!vmc)
		return NULL;

	int count;

	if (public_only) {
		count = 0;

		for (int i = 0; i < vmc->class->methods_count; i++) {
			struct vm_method *vmm = &vmc->methods[i];

			if (vm_method_is_public(vmm))
				count ++;
		}
	} else {
		count = vmc->class->methods_count;
	}

	struct vm_object *array
		= vm_object_alloc_array(vm_array_of_java_lang_reflect_Method,
					count);
	if (!array) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	int index = 0;

	for (int i = 0; i < vmc->class->methods_count; i++) {
		struct vm_method *vmm = &vmc->methods[i];

		if (public_only && !vm_method_is_public(vmm))
			continue;

		struct vm_object *method
			= vm_object_alloc(vm_java_lang_reflect_Method);

		if (!method) {
			NOT_IMPLEMENTED;
			return NULL;
		}

		struct vm_object *name_object
			= vm_object_alloc_string_from_c(vmm->name);

		if (!name_object) {
			NOT_IMPLEMENTED;
			return NULL;
		}

		field_set_object(method, vm_java_lang_reflect_Method_declaringClass,
				 clazz);
		field_set_object(method, vm_java_lang_reflect_Method_name,
				 name_object);
		field_set_int32(method, vm_java_lang_reflect_Method_slot,
				i);

		array_set_field_ptr(array, index++, method);
	}

	return array;
}

struct vm_object *
native_vmclass_get_declared_constructors(struct vm_object *clazz,
					 jboolean public_only)
{
	struct vm_class *vmc;

	vmc = to_vmclass(clazz);
	if (!vmc)
		return NULL;

	int count = 0;

	for (int i = 0; i < vmc->class->methods_count; i++) {
		struct vm_method *vmm = &vmc->methods[i];

		if (vm_method_is_constructor(vmm) &&
		    (vm_method_is_public(vmm) || !public_only))
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

		if (!vm_method_is_constructor(vmm) ||
		    (!vm_method_is_public(vmm) && public_only))
			continue;

		struct vm_object *ctor
			= vm_object_alloc(vm_java_lang_reflect_Constructor);

		if (!ctor) {
			NOT_IMPLEMENTED;
			return NULL;
		}

		field_set_object(ctor, vm_java_lang_reflect_Constructor_clazz,
				 clazz);
		field_set_int32(ctor, vm_java_lang_reflect_Constructor_slot,
				i);

		array_set_field_ptr(array, index++, ctor);
	}

	return array;
}

static struct vm_class *vm_type_to_class(char *type_name, enum vm_type type)
{
	switch (type) {
	case J_BOOLEAN:
		return vm_boolean_class;
	case J_CHAR:
		return vm_char_class;
	case J_FLOAT:
		return vm_float_class;
	case J_DOUBLE:
		return vm_double_class;
	case J_BYTE:
		return vm_byte_class;
	case J_SHORT:
		return vm_short_class;
	case J_INT:
		return vm_int_class;
	case J_LONG:
		return vm_long_class;
	case J_REFERENCE:
		return classloader_load(type_name);
	default:
		NOT_IMPLEMENTED;
	}

	return NULL;
}

static struct vm_object *get_method_parameter_types(struct vm_method *vmm)
{
	struct vm_object *array;
	unsigned int count;
	enum vm_type type;
	const char *type_str;
	char *type_name;
	int i;

	count = count_java_arguments(vmm->type);
	array = vm_object_alloc_array(vm_array_of_java_lang_Class, count);
	if (!array) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	type_str = vmm->type;
	i = 0;

	while ((type_str = parse_method_args(type_str, &type, &type_name))) {
		struct vm_class *class;

		class = vm_type_to_class(type_name, type);
		free(type_name);

		if (class)
			vm_class_ensure_init(class);

		if (!class || exception_occurred())
			return NULL;

		array_set_field_ptr(array, i++, class->object);
	}

	return array;
}

struct vm_object *
native_method_get_parameter_types(struct vm_object *method)
{
	struct vm_object *clazz;
	struct vm_class *class;
	struct vm_method *vmm;
	int slot;

	if (!method) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return NULL;
	}

	clazz = field_get_object(method, vm_java_lang_reflect_Method_declaringClass);
	slot = field_get_int32(method, vm_java_lang_reflect_Method_slot);

	class = vm_class_get_class_from_class_object(clazz);
	vmm = &class->methods[slot];

	return get_method_parameter_types(vmm);
}

struct vm_object *
native_constructor_get_parameter_types(struct vm_object *ctor)
{
	struct vm_object *clazz;
	struct vm_class *class;
	struct vm_method *vmm;
	int slot;

	if (!ctor) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return NULL;
	}

	clazz = field_get_object(ctor, vm_java_lang_reflect_Constructor_clazz);
	slot = field_get_int32(ctor, vm_java_lang_reflect_Constructor_slot);

	class = vm_class_get_class_from_class_object(clazz);
	vmm = &class->methods[slot];

	return get_method_parameter_types(vmm);
}

jint native_constructor_get_modifiers_internal(struct vm_object *ctor)
{
	struct vm_object *clazz;
	struct vm_class *class;
	struct vm_method *vmm;
	int slot;

	if (!ctor) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return 0;
	}

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

	class = to_vmclass(declaring_class);
	if (!class)
		return NULL;

	vmm = &class->methods[slot];

	result = vm_object_alloc(class);
	if (!result) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	/* TODO: We support only ()V constructors yet. */
	assert(args == NULL || args->array_length == 0);

	vm_call_method(vmm, result);
	return result;
}

struct vm_object *native_vmclass_get_interfaces(struct vm_object *clazz)
{
	struct vm_class *vmc;

	vmc = to_vmclass(clazz);
	if (!vmc)
		return NULL;

	struct vm_object *array
		= vm_object_alloc_array(vm_array_of_java_lang_Class,
					vmc->nr_interfaces);
	if (!array) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	for (unsigned int i = 0; i < vmc->nr_interfaces; i++) {
		vm_class_ensure_init(vmc->interfaces[i]);
		if (exception_occurred())
			return NULL;

		array_set_field_ptr(array, i, vmc->interfaces[i]->object);
	}

	return array;
}

struct vm_object *native_vmclass_get_superclass(struct vm_object *clazz)
{
	struct vm_class *vmc;

	vmc = to_vmclass(clazz);
	if (!vmc)
		return NULL;

	if (vm_class_is_array_class(vmc))
		return vm_java_lang_Object->object;

	if (vm_class_is_interface(vmc) ||
	    vm_class_is_primitive_class(vmc) ||
	    vmc == vm_java_lang_Object)
		return NULL;

	return vmc->super->object;
}

static struct vm_object *encapsulate_value(void *value_p, enum vm_type type)
{
	struct vm_object *obj;

	switch (type) {
	case J_REFERENCE:
		return *(struct vm_object **) value_p;
	case J_BOOLEAN:
		obj = vm_object_alloc(vm_java_lang_Boolean);
		if (!obj)
			goto failed_obj;
		vm_call_method(vm_java_lang_Boolean_init, obj, *(jboolean *) value_p);
		return obj;
	case J_BYTE:
		obj = vm_object_alloc(vm_java_lang_Byte);
		if (!obj)
			goto failed_obj;
		vm_call_method(vm_java_lang_Byte_init, obj, *(jbyte *) value_p);
		return obj;
	case J_CHAR:
		obj = vm_object_alloc(vm_java_lang_Character);
		if (!obj)
			goto failed_obj;
		vm_call_method(vm_java_lang_Character_init, obj, *(jchar *) value_p);
		return obj;
	case J_SHORT:
		obj = vm_object_alloc(vm_java_lang_Short);
		if (!obj)
			goto failed_obj;
		vm_call_method(vm_java_lang_Short_init, obj, *(jshort *) value_p);
		return obj;
	case J_FLOAT:
		obj = vm_object_alloc(vm_java_lang_Float);
		if (!obj)
			goto failed_obj;
		vm_call_method(vm_java_lang_Float_init, obj, *(jfloat *) value_p);
		return obj;
	case J_INT:
		obj = vm_object_alloc(vm_java_lang_Integer);
		if (!obj)
			goto failed_obj;
		vm_call_method(vm_java_lang_Integer_init, obj, *(jint *) value_p);
		return obj;
	case J_DOUBLE:
		obj = vm_object_alloc(vm_java_lang_Double);
		if (!obj)
			goto failed_obj;
		vm_call_method(vm_java_lang_Double_init, obj, *(jdouble *) value_p);
		return obj;
	case J_LONG:
		obj = vm_object_alloc(vm_java_lang_Long);
		if (!obj)
			goto failed_obj;
		vm_call_method(vm_java_lang_Long_init, obj, *(jlong *) value_p);
		return obj;
	default:
		error("invalid type");
	}

 failed_obj:
	NOT_IMPLEMENTED;
	return NULL;
}

struct vm_object *native_field_get(struct vm_object *this, struct vm_object *o)
{
	struct vm_field *vmf;
	struct vm_class *vmc;
	struct vm_object *clazz;
	unsigned int slot;
	void *value_p;

	if (!this) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return NULL;
	}

	clazz = field_get_object(this, vm_java_lang_reflect_Field_declaringClass);
	slot = field_get_int32(this, vm_java_lang_reflect_Field_slot);

	vmc = vm_class_get_class_from_class_object(clazz);

	vm_class_ensure_init(vmc);
	if (exception_occurred())
		return NULL;

	vmf = &vmc->fields[slot];

	/*
	 * TODO: "If this Field enforces access control, your runtime
	 * context is evaluated, and you may have an
	 * IllegalAccessException if you could not access this field
	 * in similar compiled code". (java/lang/reflect/Field.java)
	 */

	enum vm_type type = vm_field_type(vmf);

	if (vm_field_is_static(vmf)) {
		value_p = vmc->static_values + vmf->offset;
	} else {
		/*
		 * If o is null, you get a NullPointerException, and
		 * if it is incompatible with the declaring class of
		 * the field, you get an IllegalArgumentException.
		 */
		if (!o) {
			signal_new_exception(vm_java_lang_NullPointerException,
					     NULL);
			return NULL;
		}

		if (o->class != vmc) {
			signal_new_exception(vm_java_lang_IllegalArgumentException, NULL);
			return NULL;
		}

		value_p = &o->fields[vmf->offset];
	}

	return encapsulate_value(value_p, type);
}
