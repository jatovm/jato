/*
 * Copyright (c) 2010 Pekka Enberg
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

#include "vm/annotation.h"

#include "cafebabe/annotation.h"
#include "cafebabe/class.h"

#include "vm/classloader.h"
#include "vm/preload.h"
#include "vm/boxing.h"
#include "vm/errors.h"
#include "vm/class.h"
#include "vm/call.h"
#include "vm/die.h"
#include "vm/gc.h"

#include <stdlib.h>
#include <string.h>

struct vm_object *vm_annotation_to_object(struct vm_annotation *vma)
{
	struct vm_object *annotation;
	struct vm_type_info type;
	struct vm_class *klass;
	struct vm_object *map;
	unsigned int i;

	map = vm_object_alloc(vm_java_util_HashMap);
	if (!map)
		return rethrow_exception();

	vm_call_method_object(vm_java_util_HashMap_init, map);
	if (exception_occurred())
		return rethrow_exception();

	if (parse_type(&vma->type, &type))
		return NULL;

	klass = classloader_load(get_system_class_loader(), type.class_name);
	if (!klass)
		return rethrow_exception();

	for (i = 0; i < vma->nr_elements; i++) {
		struct vm_object *key, *value;

		key = vm_object_alloc_string_from_c(vma->elements[i].name);
		if (exception_occurred())
			return rethrow_exception();

		value = vma->elements[i].value;

		vm_call_method_object(vm_java_util_HashMap_put, map, key, value);
		if (exception_occurred())
			return rethrow_exception();
	}

	annotation = vm_call_method_object(vm_sun_reflect_annotation_AnnotationInvocationHandler_create, klass->object, map);
	if (!annotation)
		return rethrow_exception();

	return annotation;
}

struct vm_annotation *vm_annotation_parse(const struct cafebabe_class *klass, struct cafebabe_annotation *annotation)
{
	const struct cafebabe_constant_info_utf8 *type;
	struct vm_annotation *vma;

        if (cafebabe_class_constant_get_utf8(klass, annotation->type_index, &type))
                return NULL;

	vma = calloc(1, sizeof *vma);
	if (!vma)
		return NULL;

	vma->type = strndup((char *) type->bytes, type->length);
	if (!vma->type)
		goto out_free;

	vma->elements		= calloc(annotation->num_element_value_pairs, sizeof(struct vm_element_value_pair));
	if (!vma->elements)
		goto out_free;

	for (unsigned int i = 0; i < annotation->num_element_value_pairs; i++) {
		struct cafebabe_element_value_pair *ev_pair = &annotation->element_value_pairs[i];
		const struct cafebabe_constant_info_utf8 *name;
		struct vm_element_value_pair *vme;

		if (cafebabe_class_constant_get_utf8(klass, ev_pair->element_name_index, &name))
			goto out_free;

		vme		= &vma->elements[vma->nr_elements++];

		vme->name = strndup((char *) name->bytes, name->length);
		if (!vme->name)
			goto out_free;

		switch (ev_pair->value.tag) {
		case ELEMENT_TYPE_BYTE: {
			jint value;

			if (cafebabe_class_constant_get_integer(klass, ev_pair->value.value.const_value_index, &value))
				goto out_free;

			vme->value		= byte_to_object(value);
			break;
		}
		case ELEMENT_TYPE_CHAR: {
			jint value;

			if (cafebabe_class_constant_get_integer(klass, ev_pair->value.value.const_value_index, &value))
				goto out_free;

			vme->value		= char_to_object(value);
			break;
		}
		case ELEMENT_TYPE_DOUBLE: {
			jdouble value;

			if (cafebabe_class_constant_get_double(klass, ev_pair->value.value.const_value_index, &value))
				goto out_free;

			vme->value		= double_to_object(value);
			break;
		}
		case ELEMENT_TYPE_FLOAT: {
			jfloat value;

			if (cafebabe_class_constant_get_float(klass, ev_pair->value.value.const_value_index, &value))
				goto out_free;

			vme->value		= float_to_object(value);
			break;
		}
		case ELEMENT_TYPE_INTEGER: {
			jint value;

			if (cafebabe_class_constant_get_integer(klass, ev_pair->value.value.const_value_index, &value))
				goto out_free;

			vme->value		= int_to_object(value);
			break;
		}
		case ELEMENT_TYPE_LONG: {
			jlong value;

			if (cafebabe_class_constant_get_long(klass, ev_pair->value.value.const_value_index, &value))
				goto out_free;

			vme->value		= long_to_object(value);
			break;
		}
		case ELEMENT_TYPE_SHORT: {
			jint value;

			if (cafebabe_class_constant_get_integer(klass, ev_pair->value.value.const_value_index, &value))
				goto out_free;

			vme->value		= short_to_object(value);
			break;
		}
		case ELEMENT_TYPE_BOOLEAN: {
			jint value;

			if (cafebabe_class_constant_get_integer(klass, ev_pair->value.value.const_value_index, &value))
				goto out_free;

			vme->value		= boolean_to_object(value);
			break;
		}
		case ELEMENT_TYPE_STRING: {
			const struct cafebabe_constant_info_utf8 *utf8;
			struct vm_object *string;

			if (cafebabe_class_constant_get_utf8(klass, ev_pair->value.value.const_value_index, &utf8))
				goto out_free;

			string = vm_object_alloc_string_from_utf8(utf8->bytes, utf8->length);
			if (!string)
				goto out_free;

			vme->value		= string;
			break;
		}
		case ELEMENT_TYPE_ENUM_CONSTANT:
			vme->value		= NULL; /* TODO */
			break;
		case ELEMENT_TYPE_CLASS:
			vme->value		= NULL; /* TODO */
			break;
		case ELEMENT_TYPE_ANNOTATION_TYPE: {
			struct vm_annotation *child_vma;

			child_vma	= vm_annotation_parse(klass, ev_pair->value.value.annotation_value);
			if (!child_vma)
				goto out_free;

			vme->value	= vm_annotation_to_object(child_vma);
			if (!vme->value)
				goto out_free;
			break;
		}
		case ELEMENT_TYPE_ARRAY:
			vme->value		= NULL; /* TODO */
			break;
		default:
			warn("'%c' is an unknown element value pair tag", ev_pair->value.tag);
			goto out_free;
		}
	}

	return vma;

out_free:
	vm_annotation_free(vma);

	return NULL;
}

void vm_annotation_free(struct vm_annotation *vma)
{
	for (unsigned int i = 0; i < vma->nr_elements; i++) {
		struct vm_element_value_pair *vme = &vma->elements[i];

		free(vme->name);
	}
	free(vma->elements);
	free(vma->type);
}