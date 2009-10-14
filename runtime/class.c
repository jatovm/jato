/*
 * Copyright (c) 2009 Tomasz Grabiec
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

#include "runtime/class.h"
#include "jit/exception.h"

#include "vm/classloader.h"
#include "vm/reflection.h"
#include "vm/preload.h"
#include "vm/object.h"
#include "vm/class.h"
#include "vm/vm.h"

#include <stddef.h>

struct vm_object *native_vmclass_getclassloader(struct vm_object *object)
{
	struct vm_class *vmc;

	vmc = vm_object_to_vm_class(object);
	if (!vmc)
		return NULL;

	return vmc->classloader;
}

struct vm_object *native_vmclass_forname(struct vm_object *name,
					 jboolean initialize,
					 struct vm_object *loader)
{
	struct vm_class *class;
	char *class_name;

	if (!name) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return NULL;
	}

	class_name = vm_string_to_cstr(name);
	if (!class_name) {
		signal_new_exception(vm_java_lang_OutOfMemoryError, NULL);
		return NULL;
	}

	class = classloader_load(loader, class_name);
	if (!class)
		goto throw_cnf;

	if (initialize) {
		if (vm_class_ensure_init(class))
			goto throw_cnf;
	}

	return class->object;
 throw_cnf:
	signal_new_exception(vm_java_lang_ClassNotFoundException, class_name);
	free(class_name);
	return NULL;
}

struct vm_object *native_vmclass_getname(struct vm_object *object)
{
	struct vm_class *class = vm_object_to_vm_class(object);

	if (!class)
		return NULL;

	struct vm_object *obj;
	char *dot_name;

	dot_name = slash2dots(class->name);
	obj =  vm_object_alloc_string_from_c(dot_name);
	free(dot_name);

	return obj;
}

int32_t native_vmclass_is_anonymous_class(struct vm_object *object)
{
	struct vm_class *class = vm_object_to_vm_class(object);

	if (!class)
		return false;

	return vm_class_is_anonymous(class);
}

jint native_vmclass_is_assignable_from(struct vm_object *clazz_1,
					   struct vm_object *clazz_2)
{
	struct vm_class *vmc_1 = vm_object_to_vm_class(clazz_1);
	struct vm_class *vmc_2 = vm_object_to_vm_class(clazz_2);

	if (!vmc_1 || !vmc_2)
		return false;

	return vm_class_is_assignable_from(vmc_1, vmc_2);
}

int32_t native_vmclass_isarray(struct vm_object *object)
{
	struct vm_class *class = vm_object_to_vm_class(object);

	if (!class)
		return false;

	return vm_class_is_array_class(class);
}

int32_t native_vmclass_isprimitive(struct vm_object *object)
{
	struct vm_class *class = vm_object_to_vm_class(object);

	if (!class)
		return false;

	return vm_class_is_primitive_class(class);
}

jint native_vmclass_getmodifiers(struct vm_object *clazz)
{
	struct vm_class *class = vm_object_to_vm_class(clazz);
	if (!class)
		return 0;

	return class->access_flags;
}

struct vm_object *native_vmclass_getcomponenttype(struct vm_object *object)
{
	struct vm_class *class = vm_object_to_vm_class(object);

	if (!class)
		return NULL;

	if (!vm_class_is_array_class(class)) {
		warn("%s", class->name);
		return NULL;
	}

	return vm_class_get_array_element_class(class)->object;
}

jint native_vmclass_isinstance(struct vm_object *clazz,
				   struct vm_object *object)
{
	struct vm_class *class = vm_object_to_vm_class(clazz);

	if (!object || !class)
		return false;

	return vm_class_is_assignable_from(class, object->class);
}

jint native_vmclass_isinterface(struct vm_object *clazz)
{
	struct vm_class *class = vm_object_to_vm_class(clazz);
	if (!class)
		return false;

	return vm_class_is_interface(class);
}
