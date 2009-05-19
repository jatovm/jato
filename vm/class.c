/*
 * Copyright (c) 2008 Saeed Siam
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

#define _GNU_SOURCE

#include <vm/vm.h>
#include <stdlib.h>

unsigned long is_object_instance_of(struct object *obj, struct object *type)
{
	if (!obj)
		return 0;

	NOT_IMPLEMENTED;
	//return isInstanceOf(type, obj->class);
	return 0;
}

void check_null(struct object *obj)
{
	if (!obj)
		abort();
}

void check_array(struct object *obj, unsigned int index)
{
	NOT_IMPLEMENTED;

	struct classblock *cb = CLASS_CB(obj->class);

	if (!IS_ARRAY(cb))
		abort();

	if (index >= ARRAY_LEN(obj))
		abort();
}

void check_cast(struct object *obj, struct object *type)
{
	if (!obj)
		return;

	NOT_IMPLEMENTED;
	//if (!isInstanceOf(type, obj->class))
	//	abort();
}

#include <string.h>

#include <cafebabe/class.h>
#include <cafebabe/constant_pool.h>
#include <cafebabe/field_info.h>
#include <cafebabe/method_info.h>

#include <vm/class.h>
#include <vm/classloader.h>
#include <vm/field.h>
#include <vm/method.h>

int vm_class_init(struct vm_class *vmc, const struct cafebabe_class *class)
{
	vmc->class = class;

	const struct cafebabe_constant_info_class *constant_class;
	if (cafebabe_class_constant_get_class(class,
		class->this_class, &constant_class))
	{
		NOT_IMPLEMENTED;
		return -1;
	}

	const struct cafebabe_constant_info_utf8 *name;
	if (cafebabe_class_constant_get_utf8(class,
		constant_class->name_index, &name))
	{
		NOT_IMPLEMENTED;
		return -1;
	}

	vmc->name = strndup((char *) name->bytes, name->length);

	if (class->super_class) {
		const struct cafebabe_constant_info_class *constant_super;
		if (cafebabe_class_constant_get_class(class,
			class->super_class, &constant_super))
		{
			NOT_IMPLEMENTED;
			return -1;
		}

		const struct cafebabe_constant_info_utf8 *super_name;
		if (cafebabe_class_constant_get_utf8(class,
			constant_super->name_index, &super_name))
		{
			NOT_IMPLEMENTED;
			return -1;
		}

		char *super_name_str = strndup((char *) super_name->bytes,
			super_name->length);

		printf("name = '%s', super name = '%s'\n",
			vmc->name, super_name_str);

		vmc->super = classloader_load(super_name_str);
		if (!vmc->super) {
			NOT_IMPLEMENTED;
			return -1;
		}

		free(super_name_str);
	} else {
		if (!strcmp(vmc->name, "java.lang.Object")) {
			NOT_IMPLEMENTED;
			return -1;
		}

		vmc->super = NULL;
	}

	vmc->fields = malloc(sizeof(*vmc->fields) * class->fields_count);
	if (!vmc->fields) {
		NOT_IMPLEMENTED;
		return -1;
	}

	for (uint16_t i = 0; i < class->fields_count; ++i) {
		if (vm_field_init(&vmc->fields[i], vmc, i)) {
			NOT_IMPLEMENTED;
			return -1;
		}
	}

	vmc->methods = malloc(sizeof(*vmc->methods) * class->methods_count);
	if (!vmc->methods) {
		NOT_IMPLEMENTED;
		return -1;
	}

	for (uint16_t i = 0; i < class->methods_count; ++i) {
		if (vm_method_init(&vmc->methods[i], vmc, i)) {
			NOT_IMPLEMENTED;
			return -1;
		}

		if (vm_method_prepare_jit(&vmc->methods[i])) {
			NOT_IMPLEMENTED;
			return -1;
		}
	}

	return 0;
}

struct vm_class *vm_class_resolve_class(struct vm_class *vmc, uint16_t i)
{
	const struct cafebabe_constant_info_class *constant_class;
	if (cafebabe_class_constant_get_class(vmc->class,
		i, &constant_class))
	{
		NOT_IMPLEMENTED;
		return NULL;
	}

	const struct cafebabe_constant_info_utf8 *class_name;
	if (cafebabe_class_constant_get_utf8(vmc->class,
		constant_class->name_index, &class_name))
	{
		NOT_IMPLEMENTED;
		return NULL;
	}

	char *class_name_str = strndup((char *) class_name->bytes,
		class_name->length);
	if (!class_name_str) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	struct vm_class *class = classloader_load(class_name_str);
	if (!class) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	return class;
}

int vm_class_resolve_field(struct vm_class *vmc, uint16_t i,
	struct vm_class **r_vmc, char **r_name, char **r_type)
{
	const struct cafebabe_constant_info_field_ref *field;
	if (cafebabe_class_constant_get_field_ref(vmc->class, i, &field)) {
		NOT_IMPLEMENTED;
		return -1;
	}

	struct vm_class *class = vm_class_resolve_class(vmc,
		field->class_index);
	if (!class) {
		NOT_IMPLEMENTED;
		return -1;
	}

	const struct cafebabe_constant_info_name_and_type *name_and_type;
	if (cafebabe_class_constant_get_name_and_type(vmc->class,
		field->name_and_type_index, &name_and_type))
	{
		NOT_IMPLEMENTED;
		return -1;
	}

	const struct cafebabe_constant_info_utf8 *name;
	if (cafebabe_class_constant_get_utf8(vmc->class,
		name_and_type->name_index, &name))
	{
		NOT_IMPLEMENTED;
		return -1;
	}

	const struct cafebabe_constant_info_utf8 *type;
	if (cafebabe_class_constant_get_utf8(vmc->class,
		name_and_type->descriptor_index, &type))
	{
		NOT_IMPLEMENTED;
		return -1;
	}

	char *name_str = strndup((char *) name->bytes, name->length);
	if (!name_str) {
		NOT_IMPLEMENTED;
		return -1;
	}

	char *type_str = strndup((char *) type->bytes, type->length);
	if (!type_str) {
		NOT_IMPLEMENTED;
		return -1;
	}

	*r_vmc = class;
	*r_name = name_str;
	*r_type = type_str;
	return 0;
}

struct vm_field *vm_class_get_field_recursive(struct vm_class *vmc,
	const char *name, const char *type)
{
	do {
		unsigned int index = 0;
		if (!cafebabe_class_get_field(vmc->class,
			name, type, &index))
		{
			return &vmc->fields[index];
		}

		vmc = vmc->super;
	} while(vmc);

	return NULL;
}

struct vm_field *
vm_class_resolve_field_recursive(struct vm_class *vmc, uint16_t i)
{
	struct vm_class *class;
	char *name;
	char *type;
	struct vm_field *result;

	if (vm_class_resolve_field(vmc, i, &class, &name, &type)) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	result = vm_class_get_field_recursive(class, name, type);

	free(name);
	free(type);
	return result;
}

int vm_class_resolve_method(struct vm_class *vmc, uint16_t i,
	struct vm_class **r_vmc, char **r_name, char **r_type)
{
	const struct cafebabe_constant_info_method_ref *method;
	if (cafebabe_class_constant_get_method_ref(vmc->class, i, &method)) {
		NOT_IMPLEMENTED;
		return -1;
	}

	struct vm_class *class = vm_class_resolve_class(vmc,
		method->class_index);
	if (!class) {
		NOT_IMPLEMENTED;
		return -1;
	}

	const struct cafebabe_constant_info_name_and_type *name_and_type;
	if (cafebabe_class_constant_get_name_and_type(vmc->class,
		method->name_and_type_index, &name_and_type))
	{
		NOT_IMPLEMENTED;
		return -1;
	}

	const struct cafebabe_constant_info_utf8 *name;
	if (cafebabe_class_constant_get_utf8(vmc->class,
		name_and_type->name_index, &name))
	{
		NOT_IMPLEMENTED;
		return -1;
	}

	const struct cafebabe_constant_info_utf8 *type;
	if (cafebabe_class_constant_get_utf8(vmc->class,
		name_and_type->descriptor_index, &type))
	{
		NOT_IMPLEMENTED;
		return -1;
	}

	char *name_str = strndup((char *) name->bytes, name->length);
	if (!name_str) {
		NOT_IMPLEMENTED;
		return -1;
	}

	char *type_str = strndup((char *) type->bytes, type->length);
	if (!type_str) {
		NOT_IMPLEMENTED;
		return -1;
	}

	*r_vmc = class;
	*r_name = name_str;
	*r_type = type_str;
	return 0;
}

struct vm_method *vm_class_get_method_recursive(struct vm_class *vmc,
	const char *name, const char *type)
{
	do {
		unsigned int index = 0;
		if (!cafebabe_class_get_method(vmc->class,
			name, type, &index))
		{
			return &vmc->methods[index];
		}

		vmc = vmc->super;
	} while(vmc);

	return NULL;
}

struct vm_method *
vm_class_resolve_method_recursive(struct vm_class *vmc, uint16_t i)
{
	struct vm_class *class;
	char *name;
	char *type;
	struct vm_method *result;

	if (vm_class_resolve_method(vmc, i, &class, &name, &type)) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	result = vm_class_get_method_recursive(class, name, type);

	free(name);
	free(type);
	return result;
}
