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
