/*
 * Copyright (c) 2008 Saeed Siam
 * Copyright (c) 2009 Vegard Nossum
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

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

#include "cafebabe/class.h"
#include "cafebabe/constant_pool.h"
#include "cafebabe/field_info.h"
#include "cafebabe/method_info.h"
#include "cafebabe/stream.h"

#include "lib/string.h"

#include "vm/class.h"
#include "vm/classloader.h"
#include "vm/die.h"
#include "vm/fault-inject.h"
#include "vm/field.h"
#include "vm/preload.h"
#include "vm/itable.h"
#include "vm/method.h"
#include "vm/object.h"
#include "vm/stdlib.h"
#include "vm/thread.h"
#include "vm/vm.h"

#include "jit/exception.h"
#include "jit/compiler.h"
#include "jit/vtable.h"

static void
setup_vtable(struct vm_class *vmc)
{
	struct vm_class *super;
	unsigned int super_vtable_size;
	struct vtable *super_vtable;
	unsigned int vtable_size;

	super = vmc->super;

	if (super) {
		super_vtable_size = super->vtable_size;
		super_vtable = &super->vtable;
	} else {
		super_vtable_size = 0;
	}

	vtable_size = 0;
	for (uint16_t i = 0; i < vmc->class->methods_count; ++i) {
		struct vm_method *vmm = &vmc->methods[i];

		if (super) {
			struct vm_method *vmm2
				= vm_class_get_method_recursive(super,
					vmm->name, vmm->type);
			if (vmm2) {
				vmm->virtual_index = vmm2->virtual_index;
				continue;
			}
		}

		vmm->virtual_index = super_vtable_size + vtable_size;
		++vtable_size;
	}

	vmc->vtable_size = super_vtable_size + vtable_size;

	vtable_init(&vmc->vtable, vmc->vtable_size);

	/* Superclass methods */
	for (uint16_t i = 0; i < super_vtable_size; ++i)
		vtable_setup_method(&vmc->vtable, i,
			super_vtable->native_ptr[i]);

	/* Our methods */
	for (uint16_t i = 0; i < vmc->class->methods_count; ++i) {
		struct vm_method *vmm = &vmc->methods[i];

		vtable_setup_method(&vmc->vtable,
				    vmm->virtual_index,
				    vm_method_call_ptr(vmm));
	}
}

static int vm_class_link_common(struct vm_class *vmc)
{
	int err;

	err = pthread_mutex_init(&vmc->mutex, NULL);
	if (err)
		return -err;

	err = vm_monitor_init(&vmc->monitor);
	if (err)
		return -err;

	vmc->object = NULL;

	return 0;
}

/*
 * This is used for grouping fields by their type, so that we can:
 *
 *   1. put reference types first (improves GC)
 *   2. sort the rest of the fields by size (improves object layout)
 */
struct field_bucket {
	unsigned int nr;
	struct vm_field **fields;
};

static void bucket_order_fields(struct field_bucket *bucket,
	unsigned int size, unsigned int *offset)
{
	unsigned int tmp_offset = *offset;

	for (unsigned int i = 0; i < bucket->nr; ++i) {
		struct vm_field *vmf = bucket->fields[i];

		vmf->offset = tmp_offset;
		tmp_offset += size;
	}

	*offset = tmp_offset;
}

static void buckets_order_fields(struct field_bucket buckets[VM_TYPE_MAX],
	unsigned int *ref_size, unsigned int *size)
{
	unsigned int offset = *size;

	/* We need to align here, because offset might be non-zero from the
	 * parent class. */
	offset = ALIGN(offset, sizeof(void *));
	bucket_order_fields(&buckets[J_REFERENCE], sizeof(void *), &offset);

	/* Align with 8-byte boundary here. We don't need to align anything
	 * after this, since we _know_ e.g. that after 8-byte fields, we will
	 * always be 8-byte aligned, which is also always 4-byte aligned. */
	offset = ALIGN(offset, 8);

	bucket_order_fields(&buckets[J_DOUBLE], 8, &offset);
	bucket_order_fields(&buckets[J_LONG], 8, &offset);

	bucket_order_fields(&buckets[J_FLOAT], 4, &offset);
	bucket_order_fields(&buckets[J_INT], 4, &offset);

	/* XXX: Should be size = 2 */
	bucket_order_fields(&buckets[J_SHORT], 4, &offset);
	bucket_order_fields(&buckets[J_CHAR], 4, &offset);

	/* XXX: Should be size = 1 */
	bucket_order_fields(&buckets[J_BYTE], 4, &offset);
	bucket_order_fields(&buckets[J_BOOLEAN], 4, &offset);

	*size = offset;
}

int vm_class_link(struct vm_class *vmc, const struct cafebabe_class *class)
{
	int err;

	vmc->class = class;
	vmc->kind = VM_CLASS_KIND_REGULAR;

	err = vm_class_link_common(vmc);
	if (err)
		return -err;

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

	vmc->source_file_name = cafebabe_class_get_source_file_name(class);

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

		/* XXX: Circularity check */
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

	vmc->nr_interfaces = class->interfaces_count;
	vmc->interfaces
		= malloc(sizeof(*vmc->interfaces) * vmc->nr_interfaces);
	if (!vmc->interfaces) {
		NOT_IMPLEMENTED;
		return -1;
	}

	for (unsigned int i = 0; i < class->interfaces_count; ++i) {
		uint16_t idx = class->interfaces[i];

		const struct cafebabe_constant_info_class *interface;
		if (cafebabe_class_constant_get_class(class, idx, &interface)) {
			NOT_IMPLEMENTED;
			return -1;
		}

		const struct cafebabe_constant_info_utf8 *name;
		if (cafebabe_class_constant_get_utf8(class,
			interface->name_index, &name))
		{
			NOT_IMPLEMENTED;
			return -1;
		}

		char *c_name = strndup((char *) name->bytes, name->length);
		if (!c_name) {
			NOT_IMPLEMENTED;
			return -1;
		}

		struct vm_class *vmi = classloader_load(c_name);
		free(c_name);
		if (!vmi)
			return -1;

		vmc->interfaces[i] = vmi;
	}

	vmc->fields = malloc(sizeof(*vmc->fields) * class->fields_count);
	if (!vmc->fields) {
		NOT_IMPLEMENTED;
		return -1;
	}

	for (uint16_t i = 0; i < class->fields_count; ++i) {
		struct vm_field *vmf = &vmc->fields[i];

		if (vm_field_init(vmf, vmc, i)) {
			NOT_IMPLEMENTED;
			return -1;
		}
	}

	if (vmc->super) {
		vmc->static_size = vmc->super->static_size;
		vmc->object_size = vmc->super->object_size;
	} else {
		vmc->static_size = 0;
		vmc->object_size = 0;
	}

	struct field_bucket field_buckets[2][VM_TYPE_MAX];
	for (unsigned int i = 0; i < VM_TYPE_MAX; ++i) {
		field_buckets[0][i].nr = 0;
		field_buckets[1][i].nr = 0;
	}

	/* Count the number of fields for each bucket */
	for (uint16_t i = 0; i < class->fields_count; ++i) {
		struct vm_field *vmf = &vmc->fields[i];
		unsigned int stat = vm_field_is_static(vmf) ? 0 : 1;
		enum vm_type type = str_to_type(vmf->type);
		struct field_bucket *bucket = &field_buckets[stat][type];

		++bucket->nr;
	}

	/* Allocate enough space in each bucket */
	for (unsigned int i = 0; i < VM_TYPE_MAX; ++i) {
		for (unsigned int j = 0; j < 2; ++j) {
			struct field_bucket *bucket = &field_buckets[j][i];

			bucket->fields = malloc(bucket->nr * sizeof(*bucket->fields));
			bucket->nr = 0;
		}
	}

	/* Place the fields in the buckets */
	for (uint16_t i = 0; i < class->fields_count; ++i) {
		struct vm_field *vmf = &vmc->fields[i];
		unsigned int stat = vm_field_is_static(vmf) ? 0 : 1;
		enum vm_type type = str_to_type(vmf->type);
		struct field_bucket *bucket = &field_buckets[stat][type];

		bucket->fields[bucket->nr++] = vmf;
	}

	unsigned int tmp;
	buckets_order_fields(field_buckets[0], &tmp, &vmc->static_size);
	buckets_order_fields(field_buckets[1], &tmp, &vmc->object_size);

	/* XXX: only static fields, right size, etc. */
	vmc->static_values = zalloc(vmc->static_size);

	for (uint16_t i = 0; i < class->fields_count; ++i) {
		struct vm_field *vmf = &vmc->fields[i];

		if (vm_field_is_static(vmf)) {
			if (vm_field_init_static(vmf)) {
				NOT_IMPLEMENTED;
				return -1;
			}
		} else {
			vm_field_init_nonstatic(vmf);
		}
	}

	vmc->methods = malloc(sizeof(*vmc->methods) * class->methods_count);
	if (!vmc->methods) {
		NOT_IMPLEMENTED;
		return -1;
	}

	for (uint16_t i = 0; i < class->methods_count; ++i) {
		struct vm_method *vmm = &vmc->methods[i];

		if (vm_method_init(&vmc->methods[i], vmc, i)) {
			NOT_IMPLEMENTED;
			return -1;
		}

		vmm->itable_index = itable_hash(vmm);

		if (vm_method_prepare_jit(&vmc->methods[i])) {
			NOT_IMPLEMENTED;
			return -1;
		}
	}

	if (!vm_class_is_interface(vmc)) {
		setup_vtable(vmc);

		if (!vm_class_is_abstract(vmc))
			vm_itable_setup(vmc);
	}

	INIT_LIST_HEAD(&vmc->static_fixup_site_list);

	vmc->state = VM_CLASS_LINKED;
	return 0;;
}

int vm_class_link_primitive_class(struct vm_class *vmc, const char *class_name)
{
	int err;

	err = vm_class_link_common(vmc);
	if (err)
		return err;

	vmc->name = strdup(class_name);
	if (!vmc->name)
		return -ENOMEM;

	vmc->kind = VM_CLASS_KIND_PRIMITIVE;
	vmc->class = NULL;
	vmc->state = VM_CLASS_LINKED;

	vmc->super = vm_java_lang_Object;
	vmc->nr_interfaces = 0;
	vmc->interfaces = NULL;
	vmc->fields = NULL;
	vmc->methods = NULL;

	vmc->object_size = 0;
	vmc->static_size = 0;

	vmc->vtable_size = vm_java_lang_Object->vtable_size;
	vmc->vtable.native_ptr = vm_java_lang_Object->vtable.native_ptr;

	vmc->source_file_name = NULL;
	return 0;
}

int vm_class_link_array_class(struct vm_class *vmc, const char *class_name)
{
	int err;

	err = vm_class_link_common(vmc);
	if (err)
		return err;

	vmc->name = strdup(class_name);
	if (!vmc->name)
		return -ENOMEM;

	vmc->kind = VM_CLASS_KIND_ARRAY;
	vmc->class = NULL;
	vmc->state = VM_CLASS_LINKED;

	vmc->super = vm_java_lang_Object;
	/* XXX: Actually, arrays should implement Serializable as well. */
	vmc->nr_interfaces = 1;
	vmc->interfaces = &vm_java_lang_Cloneable;
	vmc->fields = NULL;
	vmc->methods = NULL;

	vmc->object_size = 0;
	vmc->static_size = 0;

	vmc->vtable_size = vm_java_lang_Object->vtable_size;
	vmc->vtable.native_ptr = vm_java_lang_Object->vtable.native_ptr;

	vmc->source_file_name = NULL;
	return 0;
}

static bool vm_class_check_class_init_fault(struct vm_class *vmc,
					    struct vm_object *arg)
{
	char * str;
	bool fault;

	str = vm_string_to_cstr(arg);
	fault = (strcmp(str, vmc->name) == 0);
	free(str);

	return fault;
}

int vm_class_ensure_object(struct vm_class *vmc)
{
	vm_monitor_lock(&vmc->monitor);

	if (vmc->object)
		goto out;

	/*
	 * Set the .object member of struct vm_class to point to
	 * the object (of type java.lang.Class) for this class.
	 */
	vmc->object = vm_object_alloc(vm_java_lang_Class);
	if (!vmc->object) {
		NOT_IMPLEMENTED;
		vm_monitor_unlock(&vmc->monitor);
		return -1;
	}

	field_set_object(vmc->object, vm_java_lang_Class_vmdata,
		(struct vm_object *)vmc);

 out:
	vm_monitor_unlock(&vmc->monitor);
	return 0;
}

int vm_class_init(struct vm_class *vmc)
{
	struct vm_object *exception;

	vm_monitor_lock(&vmc->monitor);

	if (vmc->state == VM_CLASS_INITIALIZING) {
		/* XXX: we need to break recursion. */
		if (vmc->initializing_thread == vm_thread_self())
			goto out_unlock;

		while (vmc->state == VM_CLASS_INITIALIZING)
			vm_monitor_wait(&vmc->monitor);
	}

	if (vmc->state == VM_CLASS_INITIALIZED)
		goto out_unlock;

	if (vmc->state == VM_CLASS_ERRONEOUS) {
		signal_new_exception(vm_java_lang_NoClassDefFoundError,
				     vmc->name);
		vm_monitor_unlock(&vmc->monitor);
		return -1;
	}

	assert(vmc->state == VM_CLASS_LINKED);

	vmc->state = VM_CLASS_INITIALIZING;
	vmc->initializing_thread = vm_thread_self();

	vm_monitor_unlock(&vmc->monitor);

	/* Fault injection, for testing purposes */
	if (vm_fault_enabled(VM_FAULT_CLASS_INIT)) {
		struct vm_object *arg;

		arg = vm_fault_arg(VM_FAULT_CLASS_INIT);
		if (vm_class_check_class_init_fault(vmc, arg)) {
			signal_new_exception(vm_java_lang_RuntimeException,
					     NULL);
			goto error;
		}
	}

	/* JVM spec, 2nd. ed., 2.17.1: "But before Terminator can be
	 * initialized, its direct superclass must be initialized, as well
	 * as the direct superclass of its direct superclass, and so on,
	 * recursively." */
	if (vmc->super) {
		int ret = vm_class_ensure_init(vmc->super);
		if (ret)
			goto error;
	}

	vm_class_ensure_object(vmc);

	if (vmc->class) {
		/* XXX: Make sure there's at most one of these. */
		for (uint16_t i = 0; i < vmc->class->methods_count; ++i) {
			if (strcmp(vmc->methods[i].name, "<clinit>"))
				continue;

			void (*clinit_trampoline)(void)
				= vm_method_trampoline_ptr(&vmc->methods[i]);

			clinit_trampoline();
			if (exception_occurred())
				goto error;
		}
	}

	vm_monitor_lock(&vmc->monitor);
	vmc->state = VM_CLASS_INITIALIZED;
	vm_monitor_notify_all(&vmc->monitor);
	vm_monitor_unlock(&vmc->monitor);

	return 0;

 out_unlock:
	vm_monitor_unlock(&vmc->monitor);
	return 0;

 error:
	exception = exception_occurred();

	if (!vm_object_is_instance_of(exception, vm_java_lang_Error)) {
		signal_new_exception_with_cause(
			vm_java_lang_ExceptionInInitializerError,
			exception,
			vmc->name);
	}

	vm_monitor_lock(&vmc->monitor);
	vmc->state = VM_CLASS_ERRONEOUS;
	vm_monitor_notify_all(&vmc->monitor);
	vm_monitor_unlock(&vmc->monitor);

	return -1;
}

struct vm_class *vm_class_resolve_class(const struct vm_class *vmc, uint16_t i)
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

int vm_class_resolve_field(const struct vm_class *vmc, uint16_t i,
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

struct vm_field *vm_class_get_field(const struct vm_class *vmc,
	const char *name, const char *type)
{
	if (vmc->kind != VM_CLASS_KIND_REGULAR)
		return NULL;

	unsigned int index = 0;
	if (!cafebabe_class_get_field(vmc->class, name, type, &index))
		return &vmc->fields[index];

	return NULL;
}

struct vm_field *vm_class_get_field_recursive(const struct vm_class *vmc,
	const char *name, const char *type)
{
	/* See JVM Spec, 2nd ed., 5.4.3.2 "Field Resolution" */
	do {
		struct vm_field *vmf = vm_class_get_field(vmc, name, type);
		if (vmf)
			return vmf;

		for (unsigned int i = 0; i < vmc->nr_interfaces; ++i) {
			vmf = vm_class_get_field_recursive(
				vmc->interfaces[i], name, type);
			if (vmf)
				return vmf;
		}

		vmc = vmc->super;
	} while(vmc);

	return NULL;
}

struct vm_field *
vm_class_resolve_field_recursive(const struct vm_class *vmc, uint16_t i)
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

int vm_class_resolve_method(const struct vm_class *vmc, uint16_t i,
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

struct vm_method *vm_class_get_method(const struct vm_class *vmc,
	const char *name, const char *type)
{
	if (vmc->kind != VM_CLASS_KIND_REGULAR)
		return NULL;

	unsigned int index = 0;
	if (!cafebabe_class_get_method(vmc->class, name, type, &index))
		return &vmc->methods[index];

	return NULL;
}

struct vm_method *vm_class_get_method_recursive(const struct vm_class *vmc,
	const char *name, const char *type)
{
	do {
		struct vm_method *vmf = vm_class_get_method(vmc, name, type);
		if (vmf)
			return vmf;

		vmc = vmc->super;
	} while(vmc);

	return NULL;
}

static struct vm_method *vm_class_get_interface_method_recursive(
	const struct vm_class *vmc, const char *name, const char *type)
{
	struct vm_method *vmm = vm_class_get_method(vmc, name, type);
	if (vmm)
		return vmm;

	for (unsigned int i = 0; i < vmc->nr_interfaces; ++i) {
		vmm = vm_class_get_interface_method_recursive(
			vmc->interfaces[i], name, type);
		if (vmm)
			return vmm;
	}

	return NULL;
}

int vm_class_resolve_interface_method(const struct vm_class *vmc, uint16_t i,
	struct vm_class **r_vmc, char **r_name, char **r_type)
{
	const struct cafebabe_constant_info_interface_method_ref *method;
	if (cafebabe_class_constant_get_interface_method_ref(vmc->class, i,
		&method))
	{
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

struct vm_method *
vm_class_resolve_method_recursive(const struct vm_class *vmc, uint16_t i)
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

struct vm_method *
vm_class_resolve_interface_method_recursive(const struct vm_class *vmc,
	uint16_t i)
{
	struct vm_class *class;
	char *name;
	char *type;
	struct vm_method *result;

	if (vm_class_resolve_interface_method(vmc, i, &class, &name, &type)) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	result = vm_class_get_interface_method_recursive(class, name, type);

	free(name);
	free(type);
	return result;
}

static bool is_numeric(const char *s)
{
	for (unsigned int i = 0; i < strlen(s); i++) {
		if (!isdigit(s[i])) {
			return false;
		}
	}
	return true;
}

/* See Section 15.9.5 ("Anonymous Class Declarations") of the JLS for details.  */
bool vm_class_is_anonymous(struct vm_class *vmc)
{
	if (!vm_class_is_regular_class(vmc))
		return false;

	if (vm_class_is_abstract(vmc))
		return false;

	char *separator = strchr(vmc->name, '$');
	if (!separator)
		return false;

	size_t len = strlen(separator);
	if (len == 0)
		return false;

	return is_numeric(separator + 1);
}

/* Reference: http://java.sun.com/j2se/1.5.0/docs/api/java/lang/Class.html#isAssignableFrom(java.lang.Class) */
bool vm_class_is_assignable_from(const struct vm_class *vmc, const struct vm_class *from)
{
	if (vmc == from)
		return true;

	if (vm_class_is_array_class(vmc)) {
		if (!vm_class_is_array_class(from))
			return false;

		const struct vm_class *vmc_el
			= vm_class_get_array_element_class(vmc);
		const struct vm_class *from_el
			= vm_class_get_array_element_class(vmc);

		return vm_class_is_assignable_from(vmc_el, from_el);
	}

	if (from->super && vm_class_is_assignable_from(vmc, from->super))
		return true;

	for (unsigned int i = 0; i < from->nr_interfaces; ++i) {
		if (vm_class_is_assignable_from(vmc, from->interfaces[i]))
			return true;
	}

	return false;
}

char *vm_class_get_array_element_class_name(const char *class_name)
{
	if (class_name[0] != '[')
		return NULL;

	if (class_name[1] == 'L')
		/* Skip '[L' prefix and ';' suffix */
		return strndup(class_name + 2, strlen(class_name) - 3);

	return strdup(class_name + 1);
}

struct vm_class *
vm_class_get_array_element_class(const struct vm_class *array_class)
{
	struct vm_class *result;

	result = array_class->array_element_class;
	assert(result);

	vm_class_ensure_init(result);

	return result;
}

enum vm_type vm_class_get_storage_vmtype(const struct vm_class *class)
{
	if (class->kind != VM_CLASS_KIND_PRIMITIVE)
		return J_REFERENCE;

	return class->primitive_vm_type;
}

struct vm_class *vm_class_get_class_from_class_object(struct vm_object *clazz)
{
	return (struct vm_class*)field_get_object(clazz,
						  vm_java_lang_Class_vmdata);
}

struct vm_class *vm_class_get_array_class(struct vm_class *element_class)
{
	struct vm_class *result;
	char *name;

	if (!asprintf(&name, "[%s", element_class->name))
		return NULL;

	result = classloader_load(name);

	free(name);
	return result;
}
