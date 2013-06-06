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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cafebabe/constant_pool.h"
#include "cafebabe/stream.h"

static int
cafebabe_constant_info_utf8_init(struct cafebabe_constant_info_utf8 *utf8,
	struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &utf8->length))
		goto out;

	utf8->bytes = cafebabe_stream_malloc(s, utf8->length);
	if (!utf8->bytes)
		goto out;

	for (uint16_t i = 0; i < utf8->length; ++i) {
		if (cafebabe_stream_read_uint8(s, &utf8->bytes[i]))
			goto out_bytes;
	}

	return 0;

out_bytes:
	free(utf8->bytes);
out:
	return 1;
}

static void
cafebabe_constant_info_utf8_deinit(struct cafebabe_constant_info_utf8 *utf8)
{
	free(utf8->bytes);
}

static int
cafebabe_constant_info_integer_init(struct cafebabe_constant_info_integer *i,
	struct cafebabe_stream *s)
{
	return cafebabe_stream_read_uint32(s, &i->bytes);
}

static void
cafebabe_constant_info_integer_deinit(struct cafebabe_constant_info_integer *i)
{
}

static int
cafebabe_constant_info_float_init(struct cafebabe_constant_info_float *f,
	struct cafebabe_stream *s)
{
	return cafebabe_stream_read_uint32(s, &f->bytes);
}

static void
cafebabe_constant_info_float_deinit(struct cafebabe_constant_info_float *f)
{
}

static int
cafebabe_constant_info_long_init(struct cafebabe_constant_info_long *l,
	struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint32(s, &l->high_bytes))
		return 1;

	if (cafebabe_stream_read_uint32(s, &l->low_bytes))
		return 1;

	return 0;
}

static void
cafebabe_constant_info_long_deinit(struct cafebabe_constant_info_long *l)
{
}

static int
cafebabe_constant_info_double_init(struct cafebabe_constant_info_double *d,
	struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint32(s, &d->high_bytes))
		return 1;

	if (cafebabe_stream_read_uint32(s, &d->low_bytes))
		return 1;

	return 0;
}

static void
cafebabe_constant_info_double_deinit(struct cafebabe_constant_info_double *d)
{
}

static int
cafebabe_constant_info_class_init(struct cafebabe_constant_info_class *c,
	struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &c->name_index))
		return 1;

	return 0;
}

static void
cafebabe_constant_info_class_deinit(struct cafebabe_constant_info_class *c)
{
}

static int
cafebabe_constant_info_string_init(struct cafebabe_constant_info_string *str,
	struct cafebabe_stream *s)
{
	return cafebabe_stream_read_uint16(s, &str->string_index);
}

static void
cafebabe_constant_info_string_deinit(struct cafebabe_constant_info_string *str)
{
}

static int
cafebabe_constant_info_field_ref_init(
	struct cafebabe_constant_info_field_ref *fr,
	struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &fr->class_index))
		return 1;

	if (cafebabe_stream_read_uint16(s, &fr->name_and_type_index))
		return 1;

	return 0;
}

static void
cafebabe_constant_info_field_ref_deinit(
	struct cafebabe_constant_info_field_ref *r)
{
}

static int
cafebabe_constant_info_method_ref_init(
	struct cafebabe_constant_info_method_ref *mr,
	struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &mr->class_index))
		return 1;

	if (cafebabe_stream_read_uint16(s, &mr->name_and_type_index))
		return 1;

	return 0;
}

static void
cafebabe_constant_info_method_ref_deinit(struct
	cafebabe_constant_info_method_ref *r)
{
}

static int
cafebabe_constant_info_interface_method_ref_init(
	struct cafebabe_constant_info_interface_method_ref *r,
	struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &r->class_index))
		return 1;

	if (cafebabe_stream_read_uint16(s, &r->name_and_type_index))
		return 1;

	return 0;
}

static void
cafebabe_constant_info_interface_method_ref_deinit(
	struct cafebabe_constant_info_interface_method_ref *r)
{
}

static int
cafebabe_constant_info_name_and_type_init(
	struct cafebabe_constant_info_name_and_type *nat,
	struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &nat->name_index))
		return 1;

	if (cafebabe_stream_read_uint16(s, &nat->descriptor_index))
		return 1;

	return 0;
}

static void
cafebabe_constant_info_name_and_type_deinit(
	struct cafebabe_constant_info_name_and_type *nat)
{
}

int
cafebabe_constant_pool_init(struct cafebabe_constant_pool *cp,
	struct cafebabe_stream *s)
{
	uint8_t tag;
	if (cafebabe_stream_read_uint8(s, &tag))
		goto out;
	cp->tag = tag;

	switch (cp->tag) {
	case CAFEBABE_CONSTANT_TAG_UTF8:
		if (cafebabe_constant_info_utf8_init(&cp->utf8, s))
			goto out;

		break;
	case CAFEBABE_CONSTANT_TAG_INTEGER:
		if (cafebabe_constant_info_integer_init(&cp->integer_, s))
			goto out;

		break;
	case CAFEBABE_CONSTANT_TAG_FLOAT:
		if (cafebabe_constant_info_float_init(&cp->float_, s))
			goto out;

		break;
	case CAFEBABE_CONSTANT_TAG_LONG:
		if (cafebabe_constant_info_long_init(&cp->long_, s))
			goto out;

		break;
	case CAFEBABE_CONSTANT_TAG_DOUBLE:
		if (cafebabe_constant_info_double_init(&cp->double_, s))
			goto out;

		break;
	case CAFEBABE_CONSTANT_TAG_CLASS:
		if (cafebabe_constant_info_class_init(&cp->class, s))
			goto out;

		break;
	case CAFEBABE_CONSTANT_TAG_STRING:
		if (cafebabe_constant_info_string_init(&cp->string, s))
			goto out;

		break;
	case CAFEBABE_CONSTANT_TAG_FIELD_REF:
		if (cafebabe_constant_info_field_ref_init(&cp->field_ref, s))
			goto out;

		break;
	case CAFEBABE_CONSTANT_TAG_METHOD_REF:
		if (cafebabe_constant_info_method_ref_init(&cp->method_ref, s))
			goto out;

		break;
	case CAFEBABE_CONSTANT_TAG_INTERFACE_METHOD_REF:
		if (cafebabe_constant_info_interface_method_ref_init(
			&cp->interface_method_ref, s))
		{
			goto out;
		}

		break;
	case CAFEBABE_CONSTANT_TAG_NAME_AND_TYPE:
		if (cafebabe_constant_info_name_and_type_init(
			&cp->name_and_type, s))
		{
			goto out;
		}

		break;
	default:
		s->cafebabe_errno = CAFEBABE_ERROR_BAD_CONSTANT_TAG;
		goto out;
	}

	return 0;

out:
	return 1;
}

void
cafebabe_constant_pool_deinit(struct cafebabe_constant_pool *cp)
{
	switch (cp->tag) {
	case CAFEBABE_CONSTANT_TAG_UTF8:
		cafebabe_constant_info_utf8_deinit(&cp->utf8);
		break;
	case CAFEBABE_CONSTANT_TAG_INTEGER:
		cafebabe_constant_info_integer_deinit(&cp->integer_);
		break;
	case CAFEBABE_CONSTANT_TAG_FLOAT:
		cafebabe_constant_info_float_deinit(&cp->float_);
		break;
	case CAFEBABE_CONSTANT_TAG_LONG:
		cafebabe_constant_info_long_deinit(&cp->long_);
		break;
	case CAFEBABE_CONSTANT_TAG_DOUBLE:
		cafebabe_constant_info_double_deinit(&cp->double_);
		break;
	case CAFEBABE_CONSTANT_TAG_CLASS:
		cafebabe_constant_info_class_deinit(&cp->class);
		break;
	case CAFEBABE_CONSTANT_TAG_STRING:
		cafebabe_constant_info_string_deinit(&cp->string);
		break;
	case CAFEBABE_CONSTANT_TAG_FIELD_REF:
		cafebabe_constant_info_field_ref_deinit(&cp->field_ref);
		break;
	case CAFEBABE_CONSTANT_TAG_METHOD_REF:
		cafebabe_constant_info_method_ref_deinit(&cp->method_ref);
		break;
	case CAFEBABE_CONSTANT_TAG_INTERFACE_METHOD_REF:
		cafebabe_constant_info_interface_method_ref_deinit(
			&cp->interface_method_ref);
		break;
	case CAFEBABE_CONSTANT_TAG_NAME_AND_TYPE:
		cafebabe_constant_info_name_and_type_deinit(
			&cp->name_and_type);
		break;
	default:
		break;
	}
}

int
cafebabe_constant_info_utf8_compare(
	const struct cafebabe_constant_info_utf8 *s1, const char *s2)
{
	unsigned int s2_length = strlen(s2);

	if (s1->length != s2_length) {
		if (s1->length < s2_length)
			return -1;
		return 1;
	}

	return strncmp((const char *) s1->bytes, s2, s1->length);
}
