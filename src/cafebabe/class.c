/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <stdio.h>
#include <stdlib.h>

#include "cafebabe/attribute_info.h"
#include "cafebabe/class.h"
#include "cafebabe/constant_pool.h"
#include "cafebabe/error.h"
#include "cafebabe/field_info.h"
#include "cafebabe/method_info.h"
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

	c->constant_pool = malloc(sizeof(*c->constant_pool)
		* c->constant_pool_count);
	if (!c->constant_pool)
		goto out;

	uint16_t constant_pool_i;
	for (uint16_t i = 1; i < c->constant_pool_count; ++i) {
		if (cafebabe_constant_pool_init(&c->constant_pool[i], s)) {
			constant_pool_i = i;
			goto out_constant_pool_init;
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

	c->interfaces = malloc(sizeof(*c->interfaces)
		* c->interfaces_count);
	if (!c->interfaces)
		goto out_constant_pool_init;

	for (uint16_t i = 0; i < c->interfaces_count; ++i) {
		if (cafebabe_stream_read_uint16(s, &c->interfaces[i]))
			goto out_interfaces_alloc;
	}

	if (cafebabe_stream_read_uint16(s, &c->fields_count))
		goto out_interfaces_alloc;

	c->fields = malloc(sizeof(*c->fields) * c->fields_count);
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

	c->methods = malloc(sizeof(*c->methods) * c->methods_count);
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

	if (cafebabe_stream_read_uint16(s, &c->attributes_count))
		goto out_methods_init;

	c->attributes = malloc(sizeof(*c->attributes) * c->attributes_count);
	if (!c->attributes)
		goto out_methods_init;

	uint16_t attributes_i;
	for (uint16_t i = 0; i < c->attributes_count; ++i) {
		if (cafebabe_attribute_info_init(&c->attributes[i], s)) {
			attributes_i = i;
			goto out_attributes_init;
		}
	}
	attributes_i = c->attributes_count;

	if (!cafebabe_stream_eof(s)) {
		s->cafebabe_errno = CAFEBABE_ERROR_EXPECTED_EOF;
		goto out_attributes_init;
	}

	/* Success */
	return 0;

	/* Error handling (partial deinitialization) */
out_attributes_init:
	for (uint16_t i = 0; i < attributes_i; ++i)
		cafebabe_attribute_info_deinit(&c->attributes[i]);
	free(c->attributes);
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

	for (uint16_t i = 0; i < c->attributes_count; ++i)
		cafebabe_attribute_info_deinit(&c->attributes[i]);
	free(c->attributes);
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
cafebabe_class_get_field(const struct cafebabe_class *c,
	const char *name, const char *descriptor,
	const struct cafebabe_field_info **r)
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

		*r = &c->fields[i];
		return 0;
	}

	/* Not found */
	return 1;
}

int
cafebabe_class_get_method(const struct cafebabe_class *c,
	const char *name, const char *descriptor,
	const struct cafebabe_method_info **r)
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

		*r = &c->methods[i];
		return 0;
	}

	/* Not found */
	return 1;
}
