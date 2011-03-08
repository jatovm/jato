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

#include "vm/class.h"

#include "cafebabe/enclosing_method_attribute.h"
#include "cafebabe/inner_classes_attribute.h"
#include "cafebabe/annotations_attribute.h"
#include "cafebabe/constant_pool.h"
#include "cafebabe/method_info.h"
#include "cafebabe/field_info.h"
#include "cafebabe/stream.h"
#include "cafebabe/class.h"

#include "jit/exception.h"
#include "jit/compiler.h"
#include "jit/vtable.h"

#include "vm/fault-inject.h"
#include "vm/classloader.h"
#include "vm/annotation.h"
#include "vm/gc.h"
#include "vm/preload.h"
#include "vm/errors.h"
#include "vm/itable.h"
#include "vm/method.h"
#include "vm/object.h"
#include "vm/stdlib.h"
#include "vm/thread.h"
#include "vm/field.h"
#include "vm/die.h"
#include "vm/vm.h"

#include "lib/string.h"
#include "lib/array.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <ctype.h>

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
	for (uint16_t i = 0; i < vmc->nr_methods; ++i) {
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
	for (uint16_t i = 0; i < vmc->nr_methods; ++i) {
		struct vm_method *vmm = &vmc->methods[i];

		vtable_setup_method(&vmc->vtable,
				    vmm->virtual_index,
				    vm_method_call_ptr(vmm));
	}
}

/*
 * Set the .object member of struct vm_class to point to the object
 * of type java.lang.Class for this class.
 */
int vm_class_setup_object(struct vm_class *vmc)
{
	vmc->object = vm_object_alloc(vm_java_lang_Class);
	if (!vmc->object) {
		throw_oom_error();
		return -1;
	}

	field_set_object(vmc->object, vm_java_lang_Class_vmdata,
			 (struct vm_object *)vmc);
	return 0;
}

static int vm_class_link_common(struct vm_class *vmc)
{
	int err;

	compile_lock_init(&vmc->cl, true);

	err = pthread_mutex_init(&vmc->mutex, NULL);
	if (err)
		return -err;

	if (preload_finished)
		return vm_class_setup_object(vmc);

	/* If classes and fields were not preloaded yet then we can't
	 * allocate object for this class. Set .object to a dummy object
	 * for class synchronization to work. This will be replaced
	 * after preloading is finished. */
	static struct vm_object dummy_object;
	vmc->object = &dummy_object;

	return vm_preload_add_class_fixup(vmc);
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

static int insert_interface_method(struct vm_class *vmc,
				   struct array *extra_methods,
				   struct vm_method *vmm)
{
	/* We need this "manual" recursive lookup because we haven't
	 * initialized this class' list of methods yet... */
	unsigned int idx = 0;
	if (!cafebabe_class_get_method(vmc->class, vmm->name, vmm->type, &idx))
		return 0;

	if (!vmc->super)
		return 0;
	if (vm_class_get_method_recursive(vmc->super, vmm->name, vmm->type))
		return 0;

	return array_append(extra_methods, vmm);
}

static int compare_method_signatures(const void *a, const void *b)
{
	const struct vm_method *x = *(const struct vm_method **) a;
	const struct vm_method *y = *(const struct vm_method **) b;

	int name = strcmp(x->name, y->name);
	if (name)
		return name;

	int type = strcmp(x->type, y->type);
	if (type)
		return type;

	return 0;
}

static void free_buckets(int rows, int cols, struct field_bucket field_buckets[rows][cols])
{
	for (int i = 0; i < cols; ++i) {
		for (int j = 0; j < rows; ++j) {
			struct field_bucket *bucket = &field_buckets[j][i];

			vm_free(bucket->fields);
		}
	}
}

int vm_class_link(struct vm_class *vmc, const struct cafebabe_class *class)
{
	const struct cafebabe_constant_info_class *constant_class;
	const struct cafebabe_constant_info_utf8 *name;
	unsigned int nr_inner_classes;

	vmc->class = class;
	vmc->kind = VM_CLASS_KIND_REGULAR;

	if (vm_class_link_common(vmc))
		return -1;

	if (cafebabe_class_constant_get_class(class, class->this_class, &constant_class))
		return -1;

	if (cafebabe_class_constant_get_utf8(class, constant_class->name_index, &name))
		return -1;

	vmc->name = strndup((char *) name->bytes, name->length);

	vmc->access_flags = class->access_flags;

	vmc->source_file_name = cafebabe_class_get_source_file_name(class);

	if (class->super_class) {
		const struct cafebabe_constant_info_class *constant_super;
		const struct cafebabe_constant_info_utf8 *super_name;

		if (cafebabe_class_constant_get_class(class, class->super_class, &constant_super))
			goto error_free_name;

		if (cafebabe_class_constant_get_utf8(class, constant_super->name_index, &super_name))
			goto error_free_name;

		char *super_name_str = strndup((char *) super_name->bytes, super_name->length);

		/* XXX: Circularity check */
		vmc->super = classloader_load(vmc->classloader, super_name_str);
		free(super_name_str);

		if (!vmc->super)
			goto error_free_name;
	} else {
		if (!strcmp(vmc->name, "java.lang.Object"))
			goto error_free_name;

		vmc->super = NULL;
	}

	vmc->nr_interfaces = class->interfaces_count;
	vmc->interfaces = malloc(sizeof(*vmc->interfaces) * vmc->nr_interfaces);
	if (!vmc->interfaces)
		goto error_free_name;

	for (unsigned int i = 0; i < class->interfaces_count; ++i) {
		const struct cafebabe_constant_info_class *interface;
		const struct cafebabe_constant_info_utf8 *name;
		uint16_t idx = class->interfaces[i];

		if (cafebabe_class_constant_get_class(class, idx, &interface))
			goto error_free_interfaces;

		if (cafebabe_class_constant_get_utf8(class, interface->name_index, &name))
			goto error_free_interfaces;

		char *c_name = strndup((char *) name->bytes, name->length);
		if (!c_name)
			goto error_free_interfaces;

		struct vm_class *vmi = classloader_load(vmc->classloader, c_name);
		free(c_name);
		if (!vmi)
			goto error_free_interfaces;

		vmc->interfaces[i] = vmi;
	}

	vmc->fields = vm_alloc(sizeof(*vmc->fields) * class->fields_count);
	if (!vmc->fields)
		goto error_free_interfaces;

	for (uint16_t i = 0; i < class->fields_count; ++i) {
		struct vm_field *vmf = &vmc->fields[i];

		if (vm_field_init(vmf, vmc, i))
			goto error_free_fields;
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
		enum vm_type type = vmf->type_info.vm_type;
		struct field_bucket *bucket = &field_buckets[stat][type];

		++bucket->nr;
	}

	/* Allocate enough space in each bucket */
	for (unsigned int i = 0; i < VM_TYPE_MAX; ++i) {
		for (unsigned int j = 0; j < 2; ++j) {
			struct field_bucket *bucket = &field_buckets[j][i];

			bucket->fields = vm_alloc(bucket->nr * sizeof(*bucket->fields));
			bucket->nr = 0;
		}
	}

	/* Place the fields in the buckets */
	for (uint16_t i = 0; i < class->fields_count; ++i) {
		struct vm_field *vmf = &vmc->fields[i];
		unsigned int stat = vm_field_is_static(vmf) ? 0 : 1;
		enum vm_type type = vmf->type_info.vm_type;
		struct field_bucket *bucket = &field_buckets[stat][type];

		bucket->fields[bucket->nr++] = vmf;
	}

	unsigned int tmp;
	buckets_order_fields(field_buckets[0], &tmp, &vmc->static_size);
	buckets_order_fields(field_buckets[1], &tmp, &vmc->object_size);

	/* XXX: only static fields, right size, etc. */
	vmc->static_values = vm_zalloc(vmc->static_size);
	if (!vmc->static_values)
		goto error_free_buckets;

	for (uint16_t i = 0; i < class->fields_count; ++i) {
		struct vm_field *vmf = &vmc->fields[i];

		if (vm_field_is_static(vmf)) {
			if (vm_field_init_static(vmf))
				goto error_free_static_values;
		} else {
			vm_field_init_nonstatic(vmf);
		}
	}

	struct array extra_methods;
	array_init(&extra_methods);

	/* The array is temporary anyway, so there's no harm in allocating a
	 * bit more just in case. If it's too little, the array will expand. */
	array_resize(&extra_methods, 64);

	/* If in any of the superinterfaces we find a method which is not
	 * defined in this class file, we need to add a "miranda" method.
	 * Note that we don't need to do this recursively for all super-
	 * interfaces because they will have already done this very same
	 * procedure themselves. */
	for (unsigned int i = 0; i < class->interfaces_count; ++i) {
		struct vm_class *vmi = vmc->interfaces[i];

		for (unsigned int j = 0; j < vmi->nr_methods; ++j) {
			struct vm_method *vmm = &vmi->methods[j];
			int err;

			err = insert_interface_method(vmc, &extra_methods, vmm);
			if (err)
				goto error_free_static_values;
		}
	}

	/* We need to weed out duplicate signatures in order to avoid a
	 * situation where two interfaces define the same method and a class
	 * implements both interfaces. We shouldn't add two methods with the
	 * same signature to the same class. */
	array_qsort(&extra_methods, &compare_method_signatures);
	array_unique(&extra_methods, &compare_method_signatures);

	vmc->nr_methods = class->methods_count + extra_methods.size;

	vmc->methods = vm_alloc(sizeof(*vmc->methods) * vmc->nr_methods);
	if (!vmc->methods)
		goto error_free_static_values;

	for (uint16_t i = 0; i < class->methods_count; ++i) {
		struct vm_method *vmm = &vmc->methods[i];

		if (vm_method_init(vmm, vmc, i))
			goto error_free_methods;
	}

	for (unsigned int i = 0; i < extra_methods.size; ++i) {
		struct vm_method *vmm = &vmc->methods[class->methods_count + i];

		if (vm_method_init_from_interface(vmm, vmc, class->methods_count + i, extra_methods.ptr[i]))
			goto error_free_methods;
	}

	for (uint16_t i = 0; i < vmc->nr_methods; ++i) {
		struct vm_method *vmm = &vmc->methods[i];

		vmm->itable_index = itable_hash(vmm);

		if (vm_method_prepare_jit(vmm))
			goto error_free_methods;
	}

	array_destroy(&extra_methods);

	if (!vm_class_is_interface(vmc)) {
		setup_vtable(vmc);

		if (!vm_class_is_abstract(vmc))
			vm_itable_setup(vmc);
	}

	INIT_LIST_HEAD(&vmc->static_fixup_site_list);

	struct cafebabe_inner_classes_attribute inner_classes_attribute;

	memset(&inner_classes_attribute, 0, sizeof(inner_classes_attribute));
	if (cafebabe_read_inner_classes_attribute(class, &class->attributes, &inner_classes_attribute))
		return -1;

	nr_inner_classes = 0;
	for (unsigned int i = 0; i < inner_classes_attribute.number_of_classes; i++) {
		struct cafebabe_inner_class *inner = &inner_classes_attribute.inner_classes[i];

		if (class->this_class == inner->outer_class_info_index)
			nr_inner_classes++;
	}

	vmc->inner_classes = vm_alloc(sizeof(*vmc->inner_classes) * nr_inner_classes);
	if (!vmc->inner_classes)
		goto error_free_methods;

	for (unsigned int i = 0; i < inner_classes_attribute.number_of_classes; i++) {
		struct cafebabe_inner_class *inner = &inner_classes_attribute.inner_classes[i];

		if (class->this_class == inner->inner_class_info_index) {
			vmc->declaring_class = vmc->enclosing_class = vm_class_resolve_class(vmc, inner->outer_class_info_index);

			vmc->inner_class_access_flags	= inner->inner_class_access_flags;
		} else if (class->this_class == inner->outer_class_info_index) {
			vmc->inner_classes[vmc->nr_inner_classes++] = inner->inner_class_info_index;
		}
	}

	struct cafebabe_annotations_attribute annotations_attribute;

	if (cafebabe_read_annotations_attribute(class, &class->attributes, &annotations_attribute))
		goto error_free_inner_classes;

	vmc->annotations = vm_alloc(sizeof(struct vm_annotation *) * annotations_attribute.num_annotations);
	if (!vmc->annotations)
		goto error_free_inner_classes;

	for (unsigned int i = 0; i < annotations_attribute.num_annotations; i++) {
		struct cafebabe_annotation *annotation = &annotations_attribute.annotations[i];
		struct vm_annotation *vma;

		vma = vm_annotation_parse(vmc, annotation);
		if (!vma)
			goto error_free_annotations;

		vmc->annotations[vmc->nr_annotations++] = vma;
	}

	cafebabe_annotations_attribute_deinit(&annotations_attribute);

	if (!cafebabe_read_enclosing_method_attribute(class, &class->attributes, &vmc->enclosing_method_attribute))
		vmc->enclosing_class = vm_class_resolve_class(vmc, vmc->enclosing_method_attribute.class_index);

	vmc->state = VM_CLASS_LINKED;
	return 0;

error_free_annotations:
	for (unsigned int i = 0; i < vmc->nr_annotations; i++) {
		struct vm_annotation *vma = vmc->annotations[i];

		vm_annotation_free(vma);
	}
	vm_free(vmc->annotations);
error_free_methods:
	vm_free(vmc->methods);
error_free_inner_classes:
	vm_free(vmc->inner_classes);
error_free_static_values:
	vm_free(vmc->static_values);
error_free_buckets:
	free_buckets(2, VM_TYPE_MAX, field_buckets);
error_free_fields:
	vm_free(vmc->fields);
error_free_interfaces:
	free(vmc->interfaces);
error_free_name:
	free(vmc->name);

	return -1;
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

	vmc->access_flags = CAFEBABE_CLASS_ACC_PUBLIC | CAFEBABE_CLASS_ACC_FINAL | CAFEBABE_CLASS_ACC_ABSTRACT;

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

int vm_class_link_array_class(struct vm_class *vmc, struct vm_class *elem_class,
			      const char *class_name)
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

	vmc->array_element_class = elem_class;
	vmc->access_flags = (elem_class->access_flags & ~CAFEBABE_CLASS_ACC_INTERFACE)
		| CAFEBABE_CLASS_ACC_FINAL | CAFEBABE_CLASS_ACC_ABSTRACT;

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
	/* Before preload is finished .object points to a common
	 * dummy object. */
	assert(preload_finished);
	return 0;
}

int vm_class_init(struct vm_class *vmc)
{
	struct vm_object *exception;
	enum compile_lock_status status;

	status = compile_lock_enter(&vmc->cl);
	if (status == STATUS_COMPILED_OK || status == STATUS_REENTER)
		return 0;

	if (status == STATUS_COMPILED_ERRONOUS) {
		signal_new_exception(vm_java_lang_NoClassDefFoundError,
				     vmc->name);
		return -1;
	}

	vm_object_lock(vmc->object);
	assert(vmc->state == VM_CLASS_LINKED);
	vmc->state = VM_CLASS_INITIALIZING;
	vm_object_unlock(vmc->object);

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

	vm_object_lock(vmc->object);
	vmc->state = VM_CLASS_INITIALIZED;
	vm_object_notify_all(vmc->object);
	vm_object_unlock(vmc->object);

	compile_lock_leave(&vmc->cl, STATUS_COMPILED_OK);
	return 0;

 error:
	compile_lock_leave(&vmc->cl, STATUS_COMPILED_ERRONOUS);
	exception = exception_occurred();

	if (!vm_object_is_instance_of(exception, vm_java_lang_Error)) {
		signal_new_exception_with_cause(
			vm_java_lang_ExceptionInInitializerError,
			exception,
			vmc->name);
	}

	vm_object_lock(vmc->object);
	vmc->state = VM_CLASS_ERRONEOUS;
	vm_object_notify_all(vmc->object);
	vm_object_unlock(vmc->object);

	return -1;
}

struct vm_class *vm_class_resolve_class(const struct vm_class *vmc, uint16_t i)
{
	const struct cafebabe_constant_info_class *constant_class;

	if (cafebabe_class_constant_get_class(vmc->class, i, &constant_class))
		return NULL;

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

	struct vm_class *class
		= classloader_load(vmc->classloader, class_name_str);
	if (!class) {
		warn("failed to load class %s", class_name_str);
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

static int vm_class_resolve_name_and_type(const struct vm_class *vmc, uint16_t index, char **r_name, char **r_type)
{
	const struct cafebabe_constant_info_name_and_type *name_and_type;

	if (cafebabe_class_constant_get_name_and_type(vmc->class, index, &name_and_type)) {
		NOT_IMPLEMENTED;
		return -1;
	}

	const struct cafebabe_constant_info_utf8 *name;
	if (cafebabe_class_constant_get_utf8(vmc->class, name_and_type->name_index, &name))
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

	*r_name = name_str;
	*r_type = type_str;

	return 0;
}

int vm_class_resolve_method(const struct vm_class *vmc, uint16_t i,
	struct vm_class **r_vmc, char **r_name, char **r_type)
{
	const struct cafebabe_constant_info_method_ref *method;
	if (cafebabe_class_constant_get_method_ref(vmc->class, i, &method)) {
		NOT_IMPLEMENTED;
		return -1;
	}

	struct vm_class *class = vm_class_resolve_class(vmc, method->class_index);
	if (!class) {
		NOT_IMPLEMENTED;
		return -1;
	}

	if (vm_class_resolve_name_and_type(vmc, method->name_and_type_index, r_name, r_type))
		return -1;

	*r_vmc = class;

	return 0;
}

struct vm_method *vm_class_get_method(const struct vm_class *vmc,
	const char *name, const char *type)
{
	if (vmc->kind != VM_CLASS_KIND_REGULAR)
		return NULL;

	for (unsigned int i = 0; i < vmc->nr_methods; ++i) {
		struct vm_method *vmm = &vmc->methods[i];

		if (!strcmp(vmm->name, name) && !strcmp(vmm->type, type))
			return vmm;
	}

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

static struct vm_method *
missing_method(const struct vm_class *vmc, char *name, char *type, uint16_t access_flags)
{
	struct cafebabe_method_info *method;
	struct vm_method *vmm;

	method		= vm_alloc(sizeof *method);
	if (!method)
		return NULL;

	method->access_flags	= access_flags;

	vmm		= vm_alloc(sizeof *vmm);
	if (!vmm)
		goto error_free_method;

	vmm->name	= strdup(name);
	if (!vmm->name)
		goto error_free_vmm;

	vmm->type	= strdup(type);
	if (!vmm->type)
		goto error_free_name;

	vmm->method	= method;

	if (vm_method_do_init(vmm))
		goto error_free_type;

	return vmm;

error_free_type:
	free(vmm->type);
error_free_name:
	free(vmm->name);
error_free_vmm:
	free(vmm);
error_free_method:
	free(method);

	return NULL;
}

struct vm_method *
vm_class_resolve_method_recursive(const struct vm_class *vmc, uint16_t i, uint16_t access_flags)
{
	struct vm_method *result;
	struct vm_class *class;
	char *name;
	char *type;

	if (vm_class_resolve_method(vmc, i, &class, &name, &type)) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	result = vm_class_get_method_recursive(class, name, type);
	if (!result)
		result = missing_method(class, name, type, access_flags);

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

struct vm_method *vm_class_get_enclosing_method(const struct vm_class *vmc)
{
	struct vm_method *result;
	char *name;
	char *type;

	if (!vmc->enclosing_class)
		return NULL;

	if (vm_class_resolve_name_and_type(vmc, vmc->enclosing_method_attribute.method_index, &name, &type))
		return NULL;

	result = vm_class_get_method_recursive(vmc->enclosing_class, name, type);

	free(name);
	free(type);

	return result;
}

/* See Section 15.9.5 ("Anonymous Class Declarations") of the JLS for details.  */
bool vm_class_is_anonymous(const struct vm_class *vmc)
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

/* Reference: http://download.oracle.com/javase/1.5.0/docs/api/java/lang/Class.html#isAssignableFrom(java.lang.Class) */
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

	if (!asprintf(&name, "[L%s;", element_class->name))
		return throw_oom_error();

	result = classloader_load(element_class->classloader, name);
	free(name);
	return result;
}

struct vm_class *
vm_class_define(struct vm_object *classloader, const char *name,
		uint8_t *data, unsigned long len)
{
	struct cafebabe_stream stream;
	struct cafebabe_class *class;
	struct vm_class *result = NULL;

	cafebabe_stream_open_buffer(&stream, data, len);

	class = malloc(sizeof *class);
	if (cafebabe_class_init(class, &stream)) {
		signal_new_exception(vm_java_lang_ClassFormatError, NULL);
		goto out_stream;
	}

	cafebabe_stream_close_buffer(&stream);

	result = vm_alloc(sizeof *result);
	if (!result)
		return throw_oom_error();

	result->classloader = classloader;

	if (vm_class_link(result, class))
		return NULL;

	return result;

out_stream:
	cafebabe_stream_close_buffer(&stream);
	return NULL;
}
