/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cafebabe/attribute_array.h"
#include "cafebabe/attribute_info.h"
#include "cafebabe/class.h"
#include "cafebabe/constant_pool.h"
#include "cafebabe/error.h"
#include "cafebabe/field_info.h"
#include "cafebabe/method_info.h"
#include "cafebabe/source_file_attribute.h"
#include "cafebabe/stream.h"

int
cafebabe_class_init(struct cafebabe_class *c, struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint32(s, &c->magic))
		goto out;

	if (c->magic != 0xCAFEBABE) {
		s->cafebabe_errno = CAFEBABE_ERROR_BAD_MAGIC_NUMBER;
		goto out;
	}

	if (cafebabe_stream_read_uint16(s, &c->minor_version))
		goto out;

	if (cafebabe_stream_read_uint16(s, &c->major_version))
		goto out;

	if (cafebabe_stream_read_uint16(s, &c->constant_pool_count))
		goto out;

	c->constant_pool = cafebabe_stream_malloc(s,
		sizeof(*c->constant_pool) * c->constant_pool_count);
	if (!c->constant_pool)
		goto out;

	uint16_t constant_pool_i;
	for (uint16_t i = 1; i < c->constant_pool_count; ++i) {
		if (cafebabe_constant_pool_init(&c->constant_pool[i], s)) {
			constant_pool_i = i;
			goto out_constant_pool_init;
		}

		/* 8-byte constants take up two entries in the constant_pool
		 * table. See also section 4.4.5 of the JVM Specification. */
		switch (c->constant_pool[i].tag) {
		case CAFEBABE_CONSTANT_TAG_LONG:
		case CAFEBABE_CONSTANT_TAG_DOUBLE:
			++i;
			break;
		default:
			break;
		}
	}
	constant_pool_i = c->constant_pool_count;

	if (cafebabe_stream_read_uint16(s, &c->access_flags))
		goto out_constant_pool_init;

	if (cafebabe_stream_read_uint16(s, &c->this_class))
		goto out_constant_pool_init;

	if (cafebabe_stream_read_uint16(s, &c->super_class))
		goto out_constant_pool_init;

	if (cafebabe_stream_read_uint16(s, &c->interfaces_count))
		goto out_constant_pool_init;

	c->interfaces = cafebabe_stream_malloc(s,
		sizeof(*c->interfaces) * c->interfaces_count);
	if (!c->interfaces)
		goto out_constant_pool_init;

	for (uint16_t i = 0; i < c->interfaces_count; ++i) {
		if (cafebabe_stream_read_uint16(s, &c->interfaces[i]))
			goto out_interfaces_alloc;
	}

	if (cafebabe_stream_read_uint16(s, &c->fields_count))
		goto out_interfaces_alloc;

	c->fields = cafebabe_stream_malloc(s,
		sizeof(*c->fields) * c->fields_count);
	if (!c->fields)
		goto out_interfaces_alloc;

	uint16_t fields_i;
	for (uint16_t i = 0; i < c->fields_count; ++i) {
		if (cafebabe_field_info_init(&c->fields[i], s)) {
			fields_i = i;
			goto out_fields_init;
		}
	}
	fields_i = c->fields_count;

	if (cafebabe_stream_read_uint16(s, &c->methods_count))
		goto out_fields_init;

	c->methods = cafebabe_stream_malloc(s,
		sizeof(*c->methods) * c->methods_count);
	if (!c->methods)
		goto out_fields_init;

	uint16_t methods_i;
	for (uint16_t i = 0; i < c->methods_count; ++i) {
		if (cafebabe_method_info_init(&c->methods[i], s)) {
			methods_i = i;
			goto out_methods_init;
		}
	}
	methods_i = c->methods_count;

	if (cafebabe_stream_read_uint16(s, &c->attributes.count))
		goto out_methods_init;

	c->attributes.array = cafebabe_stream_malloc(s,
		sizeof(*c->attributes.array) * c->attributes.count);
	if (!c->attributes.array)
		goto out_methods_init;

	uint16_t attributes_i;
	for (uint16_t i = 0; i < c->attributes.count; ++i) {
		if (cafebabe_attribute_info_init(&c->attributes.array[i], s)) {
			attributes_i = i;
			goto out_attributes_init;
		}
	}
	attributes_i = c->attributes.count;

	if (!cafebabe_stream_eof(s)) {
		s->cafebabe_errno = CAFEBABE_ERROR_EXPECTED_EOF;
		goto out_attributes_init;
	}

	/* Success */
	return 0;

	/* Error handling (partial deinitialization) */
out_attributes_init:
	for (uint16_t i = 0; i < attributes_i; ++i)
		cafebabe_attribute_info_deinit(&c->attributes.array[i]);
	free(c->attributes.array);
out_methods_init:
	for (uint16_t i = 0; i < methods_i; ++i)
		cafebabe_method_info_deinit(&c->methods[i]);
	free(c->methods);
out_fields_init:
	for (uint16_t i = 0; i < fields_i; ++i)
		cafebabe_field_info_deinit(&c->fields[i]);
	free(c->fields);
out_interfaces_alloc:
	free(c->interfaces);
out_constant_pool_init:
	for (uint16_t i = 1; i < constant_pool_i; ++i)
		cafebabe_constant_pool_deinit(&c->constant_pool[i]);
	free(c->constant_pool);
out:
	return 1;
}

void
cafebabe_class_deinit(struct cafebabe_class *c)
{
	for (uint16_t i = 1; i < c->constant_pool_count; ++i)
		cafebabe_constant_pool_deinit(&c->constant_pool[i]);
	free(c->constant_pool);

	free(c->interfaces);

	for (uint16_t i = 0; i < c->fields_count; ++i)
		cafebabe_field_info_deinit(&c->fields[i]);
	free(c->fields);

	for (uint16_t i = 0; i < c->methods_count; ++i)
		cafebabe_method_info_deinit(&c->methods[i]);
	free(c->methods);

	for (uint16_t i = 0; i < c->attributes.count; ++i)
		cafebabe_attribute_info_deinit(&c->attributes.array[i]);
	free(c->attributes.array);
}

int
cafebabe_class_constant_index_invalid(const struct cafebabe_class *c,
	uint16_t i)
{
	if (i < 1 || i >= c->constant_pool_count)
		return 1;

	return 0;
}

int
cafebabe_class_constant_get_utf8(const struct cafebabe_class *c, uint16_t i,
	const struct cafebabe_constant_info_utf8 **r)
{
	if (cafebabe_class_constant_index_invalid(c, i))
		return 1;

	const struct cafebabe_constant_pool *pool = &c->constant_pool[i];
	if (pool->tag != CAFEBABE_CONSTANT_TAG_UTF8)
		return 1;

	*r = &pool->utf8;
	return 0;
}

int
cafebabe_class_constant_get_class(const struct cafebabe_class *c, uint16_t i,
	const struct cafebabe_constant_info_class **r)
{
	if (cafebabe_class_constant_index_invalid(c, i))
		return 1;

	const struct cafebabe_constant_pool *pool = &c->constant_pool[i];
	if (pool->tag != CAFEBABE_CONSTANT_TAG_CLASS)
		return 1;

	*r = &pool->class;
	return 0;
}

int
cafebabe_class_constant_get_field_ref(const struct cafebabe_class *c,
	uint16_t i, const struct cafebabe_constant_info_field_ref **r)
{
	if (cafebabe_class_constant_index_invalid(c, i))
		return 1;

	const struct cafebabe_constant_pool *pool = &c->constant_pool[i];
	if (pool->tag != CAFEBABE_CONSTANT_TAG_FIELD_REF)
		return 1;

	*r = &pool->field_ref;
	return 0;
}

int
cafebabe_class_constant_get_method_ref(const struct cafebabe_class *c,
	uint16_t i, const struct cafebabe_constant_info_method_ref **r)
{
	if (cafebabe_class_constant_index_invalid(c, i))
		return 1;

	const struct cafebabe_constant_pool *pool = &c->constant_pool[i];
	if (pool->tag != CAFEBABE_CONSTANT_TAG_METHOD_REF)
		return 1;

	*r = &pool->method_ref;
	return 0;
}

int
cafebabe_class_constant_get_interface_method_ref(const struct cafebabe_class *c,
	uint16_t i, const struct cafebabe_constant_info_interface_method_ref **r)
{
	if (cafebabe_class_constant_index_invalid(c, i))
		return 1;

	const struct cafebabe_constant_pool *pool = &c->constant_pool[i];
	if (pool->tag != CAFEBABE_CONSTANT_TAG_INTERFACE_METHOD_REF)
		return 1;

	*r = &pool->interface_method_ref;
	return 0;
}

int
cafebabe_class_constant_get_name_and_type(const struct cafebabe_class *c,
	uint16_t i, const struct cafebabe_constant_info_name_and_type **r)
{
	if (cafebabe_class_constant_index_invalid(c, i))
		return 1;

	const struct cafebabe_constant_pool *pool = &c->constant_pool[i];
	if (pool->tag != CAFEBABE_CONSTANT_TAG_NAME_AND_TYPE)
		return 1;

	*r = &pool->name_and_type;
	return 0;
}

int
cafebabe_class_get_field(const struct cafebabe_class *c,
	const char *name, const char *descriptor,
	unsigned int *r)
{
	for (uint16_t i = 0; i < c->fields_count; ++i) {
		const struct cafebabe_constant_info_utf8 *field_name;
		if (cafebabe_class_constant_get_utf8(c,
			c->fields[i].name_index, &field_name))
		{
			return 1;
		}

		if (cafebabe_constant_info_utf8_compare(field_name, name))
			continue;

		const struct cafebabe_constant_info_utf8 *field_descriptor;
		if (cafebabe_class_constant_get_utf8(c,
			c->fields[i].descriptor_index, &field_descriptor))
		{
			return 1;
		}

		if (cafebabe_constant_info_utf8_compare(
			field_descriptor, descriptor))
		{
			continue;
		}

		*r = i;
		return 0;
	}

	/* Not found */
	return 1;
}

int
cafebabe_class_get_method(const struct cafebabe_class *c,
	const char *name, const char *descriptor,
	unsigned int *r)
{
	for (uint16_t i = 0; i < c->methods_count; ++i) {
		const struct cafebabe_constant_info_utf8 *method_name;
		if (cafebabe_class_constant_get_utf8(c,
			c->methods[i].name_index, &method_name))
		{
			return 1;
		}

		if (cafebabe_constant_info_utf8_compare(method_name, name))
			continue;

		const struct cafebabe_constant_info_utf8 *method_descriptor;
		if (cafebabe_class_constant_get_utf8(c,
			c->methods[i].descriptor_index, &method_descriptor))
		{
			return 1;
		}

		if (cafebabe_constant_info_utf8_compare(
			method_descriptor, descriptor))
		{
			continue;
		}

		*r = i;
		return 0;
	}

	/* Not found */
	return 1;
}

char *cafebabe_class_get_source_file_name(const struct cafebabe_class *class)
{
	unsigned int source_file_attrib_index = 0;
	if (cafebabe_attribute_array_get(&class->attributes, "SourceFile",
		class, &source_file_attrib_index))
	{
		return NULL;
	}

	const struct cafebabe_attribute_info *attribute
		= &class->attributes.array[source_file_attrib_index];

	struct cafebabe_stream stream;
	cafebabe_stream_open_buffer(&stream,
		attribute->info, attribute->attribute_length);

	struct cafebabe_source_file_attribute source_file_attribute;
	if (cafebabe_source_file_attribute_init(&source_file_attribute,
		&stream))
	{
		return NULL;
	}

	cafebabe_stream_close_buffer(&stream);

	const struct cafebabe_constant_info_utf8 *file_name;
	if (cafebabe_class_constant_get_utf8(class,
		source_file_attribute.sourcefile_index, &file_name))
	{
		return NULL;
	}

	return strndup((char *) file_name->bytes, file_name->length);
}
