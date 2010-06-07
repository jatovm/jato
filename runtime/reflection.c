/*
 * Copyright (c) 2009  Tomasz Grabiec
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
 */

#include "vm/reflection.h"

#include "jit/exception.h"
#include "jit/args.h"

#include "vm/classloader.h"
#include "vm/preload.h"
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

struct vm_method *vm_object_to_vm_method(struct vm_object *method)
{
	struct vm_object *clazz;
	struct vm_class *vmc;
	int slot;

	if (vm_object_is_instance_of(method, vm_java_lang_reflect_Constructor)) {
		if (vm_java_lang_reflect_VMConstructor != NULL) {	/* Classpath 0.98 */
			clazz	= field_get_object(method, vm_java_lang_reflect_VMConstructor_clazz);
			slot	= field_get_int(method, vm_java_lang_reflect_VMConstructor_slot);
		} else {
			clazz	= field_get_object(method, vm_java_lang_reflect_Constructor_clazz);
			slot	= field_get_int(method, vm_java_lang_reflect_Constructor_slot);
		}
	} else 	if (vm_object_is_instance_of(method, vm_java_lang_reflect_Method)) {
		if (vm_java_lang_reflect_VMMethod != NULL) {	/* Classpath 0.98 */
			clazz = field_get_object(method, vm_java_lang_reflect_VMMethod_clazz);
			slot  = field_get_int(method, vm_java_lang_reflect_VMMethod_slot);
		} else {
			clazz = field_get_object(method, vm_java_lang_reflect_Method_declaringClass);
			slot  = field_get_int(method, vm_java_lang_reflect_Method_slot);
		}
	} else {
		signal_new_exception(vm_java_lang_Error, "Not a method object");
		return rethrow_exception();
	}

	vmc = vm_object_to_vm_class(clazz);
	if (!vmc)
		return NULL;

	return &vmc->methods[slot];
}

struct vm_object *
native_vmclass_get_declared_fields(struct vm_object *clazz,
				   jboolean public_only)
{
	struct vm_class *vmc;

	vmc = vm_object_to_vm_class(clazz);
	if (!vmc)
		return NULL;

	if (vm_class_is_primitive_class(vmc) || vm_class_is_array_class(vmc))
		return vm_object_alloc_array(vm_array_of_java_lang_reflect_Field, 0);

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
	if (!array)
		return rethrow_exception();

	int index = 0;

	for (int i = 0; i < vmc->class->fields_count; i++) {
		struct vm_field *vmf = &vmc->fields[i];

		if (public_only && !vm_field_is_public(vmf))
			continue;

		struct vm_object *field
			= vm_object_alloc(vm_java_lang_reflect_Field);

		if (!field)
			return rethrow_exception();

		struct vm_object *name_object
			= vm_object_alloc_string_from_c(vmf->name);

		if (!name_object)
			return rethrow_exception();

		if (vm_java_lang_reflect_VMField != NULL) {	/* Classpath 0.98 */
			struct vm_object *vm_field;

			vm_field = vm_object_alloc(vm_java_lang_reflect_VMField);
			if (!vm_field)
				return rethrow_exception();

			field_set_object(vm_field, vm_java_lang_reflect_VMField_clazz, clazz);
			field_set_object(vm_field, vm_java_lang_reflect_VMField_name, name_object);
			field_set_int(vm_field, vm_java_lang_reflect_VMField_slot, i);

			field_set_object(field, vm_java_lang_reflect_Field_f, vm_field);
		} else {
			field_set_object(field, vm_java_lang_reflect_Field_declaringClass, clazz);
			field_set_object(field, vm_java_lang_reflect_Field_name, name_object);
			field_set_int(field, vm_java_lang_reflect_Field_slot, i);
		}

		array_set_field_ptr(array, index++, field);
	}

	return array;
}

struct vm_object *
native_vmclass_get_declared_methods(struct vm_object *clazz,
				    jboolean public_only)
{
	struct vm_class *vmc;
	int count;

	vmc = vm_object_to_vm_class(clazz);
	if (!vmc)
		return NULL;

	if (vm_class_is_primitive_class(vmc) || vm_class_is_array_class(vmc))
		return vm_object_alloc_array(vm_array_of_java_lang_reflect_Method, 0);

	count = 0;
	for (int i = 0; i < vmc->class->methods_count; i++) {
		struct vm_method *vmm = &vmc->methods[i];

		if (public_only && !vm_method_is_public(vmm))
			continue;

		if (vm_method_is_special(vmm))
			continue;

		count ++;
	}

	struct vm_object *array
		= vm_object_alloc_array(vm_array_of_java_lang_reflect_Method, count);
	if (!array)
		return rethrow_exception();

	int index = 0;

	for (int i = 0; i < vmc->class->methods_count; i++) {
		struct vm_method *vmm = &vmc->methods[i];

		if (public_only && !vm_method_is_public(vmm))
			continue;

		if (vm_method_is_special(vmm))
			continue;

		struct vm_object *method
			= vm_object_alloc(vm_java_lang_reflect_Method);

		if (!method)
			return rethrow_exception();

		struct vm_object *name_object
			= vm_object_alloc_string_from_c(vmm->name);

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
			field_set_int(vm_method, vm_java_lang_reflect_VMMethod_slot, i);

			assert(vm_java_lang_reflect_Method_m != NULL);
			field_set_object(method, vm_java_lang_reflect_Method_m, vm_method);
		} else {
			field_set_object(method, vm_java_lang_reflect_Method_declaringClass, clazz);
			field_set_object(method, vm_java_lang_reflect_Method_name, name_object);
			field_set_int(method, vm_java_lang_reflect_Method_slot, i);
		}

		array_set_field_ptr(array, index++, method);
	}

	return array;
}

struct vm_object *
native_vmclass_get_declared_constructors(struct vm_object *clazz,
					 jboolean public_only)
{
	struct vm_class *vmc;

	vmc = vm_object_to_vm_class(clazz);
	if (!vmc)
		return NULL;

	if (vm_class_is_primitive_class(vmc) || vm_class_is_array_class(vmc))
		return vm_object_alloc_array(vm_array_of_java_lang_reflect_Constructor, 0);

	int count = 0;

	for (int i = 0; i < vmc->class->methods_count; i++) {
		struct vm_method *vmm = &vmc->methods[i];

		if (!vm_method_is_constructor(vmm) ||
		    (!vm_method_is_public(vmm) && public_only))
			continue;

		count++;
	}

	struct vm_object *array;

	array = vm_object_alloc_array(vm_array_of_java_lang_reflect_Constructor,
				      count);
	if (!array)
		return rethrow_exception();

	int index = 0;

	for (int i = 0; i < vmc->class->methods_count; i++) {
		struct vm_method *vmm = &vmc->methods[i];

		if (!vm_method_is_constructor(vmm) ||
		    (!vm_method_is_public(vmm) && public_only))
			continue;

		struct vm_object *ctor
			= vm_object_alloc(vm_java_lang_reflect_Constructor);

		if (!ctor)
			return rethrow_exception();

		if (vm_java_lang_reflect_VMConstructor != NULL) { /* Classpath 0.98 */
			struct vm_object *vm_ctor;

			vm_ctor = vm_object_alloc(vm_java_lang_reflect_VMConstructor);
			if (!vm_ctor)
				return rethrow_exception();

			field_set_object(vm_ctor, vm_java_lang_reflect_VMConstructor_clazz, clazz);
			field_set_int(vm_ctor, vm_java_lang_reflect_VMConstructor_slot, i);

			field_set_object(ctor, vm_java_lang_reflect_Constructor_cons, vm_ctor);
		} else {
			field_set_object(ctor, vm_java_lang_reflect_Constructor_clazz, clazz);
			field_set_int(ctor, vm_java_lang_reflect_Constructor_slot, i);
		}
		array_set_field_ptr(array, index++, ctor);
	}

	return array;
}

static struct vm_class *
vm_type_to_class(struct vm_object *classloader, struct vm_type_info *type_info)
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
	case J_RETURN_ADDRESS:
	case VM_TYPE_MAX:
		error("invalid type");
	}

	return NULL;
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

	if (vm_java_lang_reflect_VMConstructor != NULL) {	/* Classpath 0.98 */
		clazz	= field_get_object(ctor, vm_java_lang_reflect_VMConstructor_clazz);
		slot	= field_get_int(ctor, vm_java_lang_reflect_VMConstructor_slot);
	} else {
		clazz	= field_get_object(ctor, vm_java_lang_reflect_Constructor_clazz);
		slot	= field_get_int(ctor, vm_java_lang_reflect_Constructor_slot);
	}

	class	= vm_class_get_class_from_class_object(clazz);
	vmm	= &class->methods[slot];

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

struct vm_object *native_vmclass_get_interfaces(struct vm_object *clazz)
{
	struct vm_class *vmc;

	vmc = vm_object_to_vm_class(clazz);
	if (!vmc)
		return NULL;

	struct vm_object *array
		= vm_object_alloc_array(vm_array_of_java_lang_Class,
					vmc->nr_interfaces);
	if (!array)
		return rethrow_exception();

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

	vmc = vm_object_to_vm_class(clazz);
	if (!vmc)
		return NULL;

	if (vm_class_is_array_class(vmc))
		return vm_java_lang_Object->object;

	if (vm_class_is_interface(vmc) ||
	    vm_class_is_primitive_class(vmc) ||
	    vmc == vm_java_lang_Object)
		return NULL;

	vm_class_ensure_object(vmc->super);
	return vmc->super->object;
}

static int unwrap(void *field_ptr, enum vm_type type,
		  struct vm_object *value)
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
	case J_VOID:
	case J_RETURN_ADDRESS:
	case VM_TYPE_MAX:
		error("unexpected type");
	}

	return 0;
}

static struct vm_object *wrap(union jvalue *value, enum vm_type vm_type)
{
	switch (vm_type) {
	case J_VOID:
		return NULL;
	case J_REFERENCE:
		return value->l;
	case J_BOOLEAN:
		return vm_call_method_object(vm_java_lang_Boolean_valueOf, value->z);
	case J_BYTE:
		return vm_call_method_object(vm_java_lang_Byte_valueOf, value->b);
	case J_CHAR:
		return vm_call_method_object(vm_java_lang_Character_valueOf, value->c);
	case J_SHORT:
		return vm_call_method_object(vm_java_lang_Short_valueOf, value->s);
	case J_LONG:
		return vm_call_method_object(vm_java_lang_Long_valueOf, value->j);
	case J_INT:
		return vm_call_method_object(vm_java_lang_Integer_valueOf, value->i);
	case J_FLOAT:
		return vm_call_method_object(vm_java_lang_Float_valueOf, value->f);
	case J_DOUBLE:
		return vm_call_method_object(vm_java_lang_Double_valueOf, value->d);
	case J_RETURN_ADDRESS:
	case VM_TYPE_MAX:
		die("unexpected type");
	}

	return NULL;
}

static void *field_get_value(struct vm_field *vmf, struct vm_object *o)
{
	void *value_p;

	/*
	 * TODO: "If this Field enforces access control, your runtime
	 * context is evaluated, and you may have an
	 * IllegalAccessException if you could not access this field
	 * in similar compiled code". (java/lang/reflect/Field.java)
	 */

	if (vm_field_is_static(vmf)) {
		value_p = vmf->class->static_values + vmf->offset;
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

		if (o->class != vmf->class) {
			signal_new_exception(vm_java_lang_IllegalArgumentException, NULL);
			return NULL;
		}

		value_p = &o->fields[vmf->offset];
	}

	return value_p;
}

struct vm_object *native_field_get(struct vm_object *this, struct vm_object *o)
{
	struct vm_field *vmf;
	enum vm_type type;
	void *value_p;

	vmf = vm_object_to_vm_field(this);
	if (!vmf)
		return NULL;

	type	= vm_field_type(vmf);
	value_p	= field_get_value(vmf, o);
	if (!value_p)
		return NULL;

	return wrap((union jvalue *) value_p, type);
}

static jlong to_jlong_value(union jvalue *value, enum vm_type vm_type)
{
	switch (vm_type) {
	case J_BYTE:
		return value->b;
	case J_CHAR:
		return value->c;
	case J_SHORT:
		return value->s;
	case J_LONG:
		return value->j;
	case J_INT:
		return value->i;
	case J_BOOLEAN:
	case J_DOUBLE:
	case J_FLOAT:
	case J_REFERENCE: {
		signal_new_exception(vm_java_lang_IllegalArgumentException, NULL);
		return 0;
	}
	case J_RETURN_ADDRESS:
	case J_VOID:
	case VM_TYPE_MAX:
		break;
	}
	die("unexpected type %d", vm_type);
}

jlong native_field_get_long(struct vm_object *this, struct vm_object *o)
{
	struct vm_field *vmf;
	union jvalue *value;
	enum vm_type type;

	vmf = vm_object_to_vm_field(this);
	if (!vmf)
		return 0;

	type	= vm_field_type(vmf);
	value	= field_get_value(vmf, o);
	if (!value)
		return 0;

	return to_jlong_value(value, type);
}

jint native_field_get_modifiers_internal(struct vm_object *this)
{
	struct vm_field *vmf;

	vmf = vm_object_to_vm_field(this);
	if (!vmf)
		return 0;

	return vmf->field->access_flags;
}

void native_field_set(struct vm_object *this, struct vm_object *o,
		      struct vm_object *value_obj)
{
	struct vm_field *vmf;

	if (!this) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return;
	}

	vmf = vm_object_to_vm_field(this);
	if (!vmf)
		return;

	/*
	 * TODO: "If this Field enforces access control, your runtime
	 * context is evaluated, and you may have an
	 * IllegalAccessException if you could not access this field
	 * in similar compiled code". (java/lang/reflect/Field.java)
	 */

	enum vm_type type = vm_field_type(vmf);

	if (vm_field_is_static(vmf)) {
		unwrap(vmf->class->static_values + vmf->offset,
				     type, value_obj);
	} else {
		/*
		 * If o is null, you get a NullPointerException, and
		 * if it is incompatible with the declaring class of
		 * the field, you get an IllegalArgumentException.
		 */
		if (!o) {
			signal_new_exception(vm_java_lang_NullPointerException,
					     NULL);
			return;
		}

		if (o->class != vmf->class) {
			signal_new_exception(vm_java_lang_IllegalArgumentException, NULL);
			return;
		}

		unwrap(&o->fields[vmf->offset], type, value_obj);
	}
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
		if (unwrap(&args[idx], arg->type_info.vm_type, arg_obj))
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
	return wrap(&result, vmm->return_type.vm_type);
}

static struct vm_object *
call_static_method(struct vm_method *vmm, struct vm_object *args_array)
{
	unsigned long args[vmm->args_count];
	union jvalue result;

	if (marshall_call_arguments(vmm, args, args_array))
		return NULL;

	vm_call_method_a(vmm, args, &result);
	return wrap(&result, vmm->return_type.vm_type);
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

struct vm_object *native_field_gettype(struct vm_object *this)
{
	struct vm_field *vmf;

	if (!this) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return 0;
	}

	vmf = vm_object_to_vm_field(this);
	if (!vmf)
		return 0;

	struct vm_class *vmc = vm_type_to_class(vmf->class->classloader,
						&vmf->type_info);
	if (vm_class_ensure_init(vmc))
		return NULL;

	return vmc->object;
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
