/*
 * Copyright (c) 2009  Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "vm/reflection.h"

#include "jit/exception.h"
#include "jit/args.h"

#include "vm/classloader.h"
#include "vm/annotation.h"
#include "vm/preload.h"
#include "vm/boxing.h"
#include "vm/errors.h"
#include "vm/class.h"
#include "vm/types.h"
#include "vm/call.h"
#include "vm/die.h"

static int marshall_call_arguments(struct vm_method *vmm, unsigned long *args,
				   struct vm_object *args_array);

struct vm_class *vm_object_to_vm_class(struct vm_object *object)
{
	struct vm_class *vmc;

	if (!object)
		return throw_npe();

	vmc = vm_class_get_class_from_class_object(object);
	if (!vmc)
		return throw_npe();

	return vmc;
}

struct vm_field *vm_object_to_vm_field(struct vm_object *field)
{
	struct vm_object *class;
	struct vm_class *vmc;
	int slot;

	if (!field)
		return throw_npe();

	if (vm_java_lang_reflect_VMField != NULL) {	/* Classpath 0.98 */
		if (vm_object_is_instance_of(field, vm_java_lang_reflect_Field))
			field	= field_get_object(field, vm_java_lang_reflect_Field_f);

		class	= field_get_object(field, vm_java_lang_reflect_VMField_clazz);
		slot	= field_get_int(field, vm_java_lang_reflect_VMField_slot);
	} else {
		class	= field_get_object(field, vm_java_lang_reflect_Field_declaringClass);
		slot	= field_get_int(field, vm_java_lang_reflect_Field_slot);
	}

	vmc = vm_class_get_class_from_class_object(class);
	if (!vmc)
		return throw_npe();

	vm_class_ensure_init(vmc);
	if (exception_occurred())
		return NULL;

	return &vmc->fields[slot];
}

struct vm_object *vm_method_to_java_lang_reflect_method(struct vm_method *vmm, jobject clazz, int method_index)
{
	struct vm_object *method = vm_object_alloc(vm_java_lang_reflect_Method);

	if (!method)
		return rethrow_exception();

	struct vm_object *name_object = vm_object_alloc_string_from_c(vmm->name);

	if (!name_object)
		return rethrow_exception();

	if (vm_java_lang_reflect_VMMethod != NULL) {	/* Classpath 0.98 */
		struct vm_object *vm_method;

		vm_method = vm_object_alloc(vm_java_lang_reflect_VMMethod);
		if (!vm_method)
			return rethrow_exception();

		field_set_object(vm_method, vm_java_lang_reflect_VMMethod_clazz, clazz);
		field_set_object(vm_method, vm_java_lang_reflect_VMMethod_name, name_object);
		field_set_object(vm_method, vm_java_lang_reflect_VMMethod_m, method);
		field_set_int(vm_method, vm_java_lang_reflect_VMMethod_slot, method_index);

		assert(vm_java_lang_reflect_Method_m != NULL);
		field_set_object(method, vm_java_lang_reflect_Method_m, vm_method);
	} else {
		field_set_object(method, vm_java_lang_reflect_Method_declaringClass, clazz);
		field_set_object(method, vm_java_lang_reflect_Method_name, name_object);
		field_set_int(method, vm_java_lang_reflect_Method_slot, method_index);
	}

	return method;
}

struct vm_object *vm_field_to_java_lang_reflect_field(struct vm_field *vmf, jobject clazz, int field_index)
{
	struct vm_object *field = vm_object_alloc(vm_java_lang_reflect_Field);

	if (!field)
		return rethrow_exception();

	struct vm_object *name_object = vm_object_alloc_string_from_c(vmf->name);

	if (!name_object)
		return rethrow_exception();

	if (vm_java_lang_reflect_VMField != NULL) {	/* Classpath 0.98 */
		struct vm_object *vm_field;

		vm_field = vm_object_alloc(vm_java_lang_reflect_VMField);
		if (!vm_field)
			return rethrow_exception();

		field_set_object(vm_field, vm_java_lang_reflect_VMField_clazz, clazz);
		field_set_object(vm_field, vm_java_lang_reflect_VMField_name, name_object);
		field_set_int(vm_field, vm_java_lang_reflect_VMField_slot, field_index);

		field_set_object(field, vm_java_lang_reflect_Field_f, vm_field);
	} else {
		field_set_object(field, vm_java_lang_reflect_Field_declaringClass, clazz);
		field_set_object(field, vm_java_lang_reflect_Field_name, name_object);
		field_set_int(field, vm_java_lang_reflect_Field_slot, field_index);
	}

	return field;
}

static struct vm_class *constructor_class(void)
{
	if (vm_java_lang_reflect_VMConstructor != NULL)
		return vm_java_lang_reflect_VMConstructor;

	return vm_java_lang_reflect_Constructor;
}

static struct vm_field *constructor_clazz_field(void)
{
	if (vm_java_lang_reflect_VMConstructor != NULL)	/* Classpath 0.98 */
		return vm_java_lang_reflect_VMConstructor_clazz;

	return vm_java_lang_reflect_Constructor_clazz;
}

static struct vm_field *constructor_slot_field(void)
{
	if (vm_java_lang_reflect_VMConstructor != NULL) /* Classpath 0.98 */
		return vm_java_lang_reflect_VMConstructor_slot;

	return vm_java_lang_reflect_Constructor_slot;
}

static struct vm_class *method_class(void)
{
	if (vm_java_lang_reflect_VMMethod != NULL)	/* Classpath 0.98 */
		return vm_java_lang_reflect_VMMethod;

	return vm_java_lang_reflect_Method;
}

static struct vm_field *method_clazz_field(void)
{
	if (vm_java_lang_reflect_VMMethod != NULL)	/* Classpath 0.98 */
		return vm_java_lang_reflect_VMMethod_clazz;

	return vm_java_lang_reflect_Method_declaringClass;
}

static struct vm_field *method_slot_field(void)
{
	if (vm_java_lang_reflect_VMMethod != NULL)	/* Classpath 0.98 */
		return vm_java_lang_reflect_VMMethod_slot;

	return vm_java_lang_reflect_Method_slot;
}

struct vm_method *vm_object_to_vm_method(struct vm_object *method)
{
	struct vm_object *clazz;
	struct vm_object *vm_method_object;
	struct vm_class *vmc;
	int slot;

	if (vm_object_is_instance_of(method, constructor_class())) {
		clazz	= field_get_object(method, constructor_clazz_field());
		slot	= field_get_int(method, constructor_slot_field());
	} else if (vm_object_is_instance_of(method, method_class())) {
		clazz	= field_get_object(method, method_clazz_field());
		slot	= field_get_int(method, method_slot_field());
	} else if (vm_object_is_instance_of(method, vm_java_lang_reflect_Method)) {
		vm_method_object = field_get_object(method, vm_java_lang_reflect_Method_m);
		clazz	= field_get_object(vm_method_object, method_clazz_field());
		slot	= field_get_int(vm_method_object, method_slot_field());
	}  else {
		signal_new_exception(vm_java_lang_Error, "Not a method object: %s",
				     method->class->name);
		return rethrow_exception();
	}

	vmc = vm_object_to_vm_class(clazz);
	if (!vmc)
		return NULL;

	return &vmc->methods[slot];
}

struct vm_class *vm_type_to_class(struct vm_object *classloader, struct vm_type_info *type_info)
{
	switch (type_info->vm_type) {
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
	case J_VOID:
		return vm_void_class;
	case J_REFERENCE:
		return classloader_load(classloader, type_info->class_name);
	default:
		error("invalid type");
	}

	return NULL;
}

static struct vm_object *vm_method_get_exception_types(struct vm_method *vmm)
{
	struct vm_object *array;
	uint16_t count;
	int i;

	count	= vmm->exceptions_attribute.number_of_exceptions;

	array	= vm_object_alloc_array_of(vm_java_lang_Class, count);
	if (!array)
		return rethrow_exception();

	for (i = 0; i < count; i++) {
		struct vm_class *class = vm_class_resolve_class(vmm->class, vmm->exceptions_attribute.exceptions[i]);

		assert(class);

		array_set_field_object(array, i, class->object);
	}

	return array;
}

static struct vm_object *get_method_parameter_types(struct vm_method *vmm)

{
	struct vm_method_arg *arg;
	struct vm_object *array;
	unsigned int count;
	int i;

	count = count_java_arguments(vmm);
	array = vm_object_alloc_array(vm_array_of_java_lang_Class, count);
	if (!array)
		return rethrow_exception();

	i = 0;
	list_for_each_entry(arg, &vmm->args, list_node) {
		struct vm_class *class;

		class = vm_type_to_class(vmm->class->classloader, &arg->type_info);
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
	if (!method) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return NULL;
	}

	struct vm_method *vmm
		= vm_object_to_vm_method(method);

	if (!vmm)
		return NULL;

	return get_method_parameter_types(vmm);
}

static struct vm_method *ctor_to_method(struct vm_object *ctor)
{
	struct vm_object *clazz;
	struct vm_class *class;
	int slot;

	if (!ctor) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return NULL;
	}

	if (vm_java_lang_reflect_VMConstructor != NULL) {	/* Classpath 0.98 */
		clazz	= field_get_object(ctor, vm_java_lang_reflect_VMConstructor_clazz);
		slot	= field_get_int(ctor, vm_java_lang_reflect_VMConstructor_slot);
	} else {
		clazz	= field_get_object(ctor, vm_java_lang_reflect_Constructor_clazz);
		slot	= field_get_int(ctor, vm_java_lang_reflect_Constructor_slot);
	}

	class	= vm_class_get_class_from_class_object(clazz);

	return &class->methods[slot];
}

struct vm_object *
native_constructor_get_parameter_types(struct vm_object *ctor)
{
	struct vm_method *vmm;

	vmm	= ctor_to_method(ctor);
	if (!vmm)
		return NULL;

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

	if (vm_java_lang_reflect_VMConstructor != NULL) {	/* Classpath 0.98 */
		clazz	= field_get_object(ctor, vm_java_lang_reflect_VMConstructor_clazz);
		slot	= field_get_int(ctor, vm_java_lang_reflect_VMConstructor_slot);
	} else {
		clazz = field_get_object(ctor, vm_java_lang_reflect_Constructor_clazz);
		slot = field_get_int(ctor, vm_java_lang_reflect_Constructor_slot);
	}

	class = vm_class_get_class_from_class_object(clazz);
	vmm = &class->methods[slot];

	return vmm->method->access_flags;
}

struct vm_object *native_vmconstructor_get_exception_types(struct vm_object *ctor)
{
	struct vm_method *vmm;

	vmm	= ctor_to_method(ctor);
	if (!vmm)
		return NULL;

	return vm_method_get_exception_types(vmm);
}

struct vm_object *
native_constructor_construct_native(struct vm_object *this,
				    struct vm_object *args_array,
				    struct vm_object *declaring_class,
				    int slot)
{
	struct vm_object *result;
	struct vm_class *class;
	struct vm_method *vmm;

	class = vm_object_to_vm_class(declaring_class);
	if (!class)
		return NULL;

	vmm = &class->methods[slot];

	result = vm_object_alloc(class);
	if (!result)
		return rethrow_exception();

	unsigned long args[vmm->args_count];

	args[0] = (unsigned long) result;
	if (marshall_call_arguments(vmm, args + 1, args_array))
		return NULL;

	vm_call_method_a(vmm, args, NULL);
	return result;
}

struct vm_object *native_vmconstructor_construct(struct vm_object *this, struct vm_object *args)
{
	struct vm_object *clazz;
	jint slot;

	clazz	= field_get_object(this, vm_java_lang_reflect_VMConstructor_clazz);
	slot	= field_get_int(this, vm_java_lang_reflect_VMConstructor_slot);

	return native_constructor_construct_native(this, args, clazz, slot);
}

int object_to_jvalue(void *field_ptr, enum vm_type type, struct vm_object *value)
{
	unsigned long args[] = { (unsigned long) value };
	union jvalue result;

	switch (type) {
	case J_REFERENCE:
		*(jobject *) field_ptr = value;
		return 0;
	case J_BOOLEAN:
		vm_call_method_this_a(vm_java_lang_Boolean_booleanValue, value, args, &result);
		*(long *) field_ptr = (long) result.z;
		return 0;
	case J_CHAR:
		vm_call_method_this_a(vm_java_lang_Character_charValue, value, args, &result);
		*(long *) field_ptr = (long) result.c;
		return 0;
	case J_BYTE:
	case J_SHORT:
	case J_INT:
		/*
		 * We can handle those as int because these values are
		 * returned by ireturn anyway.
		 */
		vm_call_method_this_a(vm_java_lang_Number_intValue, value, args, &result);
		*(long *) field_ptr = result.i;
		return 0;
	case J_FLOAT:
		vm_call_method_this_a(vm_java_lang_Number_floatValue, value, args, &result);
		*(jfloat *) field_ptr = result.f;
		return 0;
	case J_LONG:
		vm_call_method_this_a(vm_java_lang_Number_longValue, value, args, &result);
		*(jlong *) field_ptr = result.j;
		return 0;
	case J_DOUBLE:
		vm_call_method_this_a(vm_java_lang_Number_doubleValue, value, args, &result);
		*(jdouble *) field_ptr = result.d;
		return 0;
	default:
		error("unexpected type");
	}

	return 0;
}

struct vm_object *enum_to_object(struct vm_object *enum_type, struct vm_object *name)
{
	return vm_call_method_object(vm_java_lang_Enum_valueOf, enum_type, name);
}

struct vm_object *boolean_to_object(jboolean value)
{
	return vm_call_method_object(vm_java_lang_Boolean_valueOf, value);
}

struct vm_object *byte_to_object(jbyte value)
{
	return vm_call_method_object(vm_java_lang_Byte_valueOf, value);
}

struct vm_object *char_to_object(jchar value)
{
	return vm_call_method_object(vm_java_lang_Character_valueOf, value);
}

struct vm_object *short_to_object(jshort value)
{
	return vm_call_method_object(vm_java_lang_Short_valueOf, value);
}

struct vm_object *int_to_object(jint value)
{
	return vm_call_method_object(vm_java_lang_Integer_valueOf, value);
}

struct vm_object *long_to_object(jlong value)
{
	return vm_call_method_object(vm_java_lang_Long_valueOf, value);
}

static uint32_t jfloat_to_u32(jfloat value)
{
	union {
		float		f;
		uint32_t	i;
	} v;

	v.f		= value;

	return v.i;
}

struct vm_object *float_to_object(jfloat value)
{
	uint32_t v = jfloat_to_u32(value);

	return vm_call_method_object(vm_java_lang_Float_valueOf, v);
}

static uint64_t jdouble_to_u64(jdouble value)
{
	union {
		double		d;
		uint64_t	l;
	} v;

	v.d		= value;

	return v.l;
}

struct vm_object *double_to_object(jdouble value)
{
	uint64_t v = jdouble_to_u64(value);

	return vm_call_method_object(vm_java_lang_Double_valueOf, v);
}

struct vm_object *jvalue_to_object(union jvalue *value, enum vm_type vm_type)
{
	switch (vm_type) {
	case J_VOID:
		return NULL;
	case J_REFERENCE:
		return value->l;
	case J_BOOLEAN:
		return boolean_to_object(value->z);
	case J_BYTE:
		return byte_to_object(value->b);
	case J_CHAR:
		return char_to_object(value->c);
	case J_SHORT:
		return short_to_object(value->s);
	case J_LONG:
		return long_to_object(value->j);
	case J_INT:
		return int_to_object(value->i);
	case J_FLOAT:
		return float_to_object(value->f);
	case J_DOUBLE:
		return double_to_object(value->d);
	default:
		die("unexpected type");
	}

	return NULL;
}

static int marshall_call_arguments(struct vm_method *vmm, unsigned long *args,
				   struct vm_object *args_array)
{
	struct vm_method_arg *arg;
	int args_array_idx;
	int idx;

	args_array_idx = 0;
	idx = 0;

	if (args_array == NULL)
		return 0;

	list_for_each_entry(arg, &vmm->args, list_node) {
		struct vm_object *arg_obj;

		arg_obj = array_get_field_ptr(args_array, args_array_idx++);
		if (object_to_jvalue(&args[idx], arg->type_info.vm_type, arg_obj))
			return -1;

		idx += get_arg_size(arg->type_info.vm_type);
	}

	return 0;
}

static struct vm_object *
call_virtual_method(struct vm_method *vmm, struct vm_object *o,
		    struct vm_object *args_array)
{
	unsigned long args[vmm->args_count];
	union jvalue result;

	args[0] = (unsigned long) o;
	if (marshall_call_arguments(vmm, args + 1, args_array))
		return NULL;

	vm_call_method_this_a(vmm, o, args, &result);
	return jvalue_to_object(&result, vmm->return_type.vm_type);
}

static struct vm_object *
call_static_method(struct vm_method *vmm, struct vm_object *args_array)
{
	unsigned long args[vmm->args_count];
	union jvalue result;

	if (marshall_call_arguments(vmm, args, args_array))
		return NULL;

	vm_call_method_a(vmm, args, &result);
	return jvalue_to_object(&result, vmm->return_type.vm_type);
}

jint native_method_get_modifiers_internal(struct vm_object *this)
{
	struct vm_method *vmm;

	vmm = vm_object_to_vm_method(this);
	if (!vmm)
		return 0;

	return vmm->method->access_flags;
}

struct vm_object *
native_method_invokenative(struct vm_object *method, struct vm_object *o,
			   struct vm_object *args,
			   struct vm_object *declaringClass,
			   jint slot)
{
	struct vm_method *vmm;
	struct vm_class *vmc;

	vmc = vm_object_to_vm_class(declaringClass);
	if (!vmc)
		return NULL;

	if (slot < 0 || slot >= vmc->class->methods_count)
		goto throw_illegal;

	vmm = &vmc->methods[slot];

	if (vm_method_is_static(vmm)) {
		return call_static_method(vmm, args);
	}

	if (!o) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return NULL;
	}

	if (!vm_object_is_instance_of(o, vmc))
		goto throw_illegal;

	return call_virtual_method(vmm, o, args);

 throw_illegal:
	signal_new_exception(vm_java_lang_IllegalArgumentException, NULL);
	return NULL;
}

struct vm_object *native_vmmethod_invoke(struct vm_object *vm_method, struct vm_object *o, struct vm_object *args)
{
	struct vm_object *method;
	struct vm_object *clazz;
	jint slot;

	method	= field_get_object(vm_method, vm_java_lang_reflect_VMMethod_m);
	clazz	= field_get_object(vm_method, vm_java_lang_reflect_VMMethod_clazz);
	slot	= field_get_int(vm_method, vm_java_lang_reflect_VMMethod_slot);

	return native_method_invokenative(method, o, args, clazz, slot);
}

struct vm_object *native_method_getreturntype(struct vm_object *method)
{
	struct vm_method *vmm = vm_object_to_vm_method(method);
	if (!vmm)
		return NULL;

	struct vm_class *vmc;

	vmc = vm_type_to_class(vmm->class->classloader, &vmm->return_type);
	if (vmc)
		vm_class_ensure_init(vmc);

	if (!vmc || exception_occurred())
		return NULL;

	return vmc->object;
}

struct vm_object *native_method_get_default_value(struct vm_object *method)
{
	return NULL;
}

struct vm_object *native_method_get_exception_types(struct vm_object *method)
{
	struct vm_method *vmm;

	vmm	= vm_object_to_vm_method(method);
	if (!vmm)
		return NULL;

	return vm_method_get_exception_types(vmm);
}

jobject native_vmarray_createobjectarray(jobject type, int dim)
{
	struct vm_object *array;
	struct vm_class *vmc;

	if (!type)
		return throw_npe();

	vmc = vm_class_get_class_from_class_object(type);
	if (!vmc)
		return throw_chained_internal_error();

	array = vm_object_alloc_array_of(vmc, dim);
	if (!array)
		return rethrow_exception();

	return array;
}

jobject java_lang_reflect_VMMethod_getParameterAnnotations(jobject klass)
{
	struct vm_object *result;
	struct vm_method *vmm;
	struct vm_method_arg *arg;
	unsigned int i,j,count;

	vmm = vm_object_to_vm_method(klass);
	if (!vmm)
		return rethrow_exception();

	count = count_java_arguments(vmm);
	result = vm_object_alloc_array(vm_array_of_java_lang_Object, count);
	if (!result)
		return rethrow_exception();

	i = 0;
	list_for_each_entry(arg, &vmm->args, list_node) {
		struct vm_class *class;
		struct vm_object *sub_result;

		class = vm_type_to_class(vmm->class->classloader, &arg->type_info);
		if (class)
			vm_class_ensure_init(class);

		if (!class)
			return NULL;

		sub_result = vm_object_alloc_array(vm_array_of_java_lang_annotation_Annotation, class->nr_annotations);
		if (!sub_result)
			rethrow_exception();

		for (j = 0; j < class->nr_annotations; j++) {
			struct vm_annotation *vma = class->annotations[i];
			struct vm_object *annotation;

			annotation = vm_annotation_to_object(vma);
			if (!annotation)
				return rethrow_exception();

			array_set_field_object(sub_result, j, annotation);
		}

		array_set_field_object(result, i, sub_result);
	}
	return result;
}
