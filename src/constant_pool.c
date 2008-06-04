#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include "cafebabe/constant_pool.h"
#include "cafebabe/stream.h"

static int
cafebabe_constant_info_utf8_init(struct cafebabe_constant_info_utf8 *utf8,
	struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &utf8->length))
		goto out;

	utf8->bytes = malloc(utf8->length);
	if (!utf8->bytes) {
		s->syscall_errno = errno;
		s->cafebabe_errno = CAFEBABE_ERROR_ERRNO;
		goto out;
	}

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

static int
cafebabe_constant_info_class_init(struct cafebabe_constant_info_class *c,
	struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &c->name_index))
		return 1;

	return 0;
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
		cp->info.utf8 = malloc(sizeof(*cp->info.utf8));
		if (!cp->info.utf8) {
			s->syscall_errno = errno;
			s->cafebabe_errno = CAFEBABE_ERROR_ERRNO;
			goto out;
		}

		if (cafebabe_constant_info_utf8_init(cp->info.utf8, s)) {
			free(cp->info.utf8);
			goto out;
		}

		break;
	case CAFEBABE_CONSTANT_TAG_INTEGER:
		break;
	case CAFEBABE_CONSTANT_TAG_FLOAT:
		break;
	case CAFEBABE_CONSTANT_TAG_LONG:
		break;
	case CAFEBABE_CONSTANT_TAG_DOUBLE:
		break;
	case CAFEBABE_CONSTANT_TAG_CLASS:
		cp->info.class = malloc(sizeof(*cp->info.class));
		if (!cp->info.class) {
			s->syscall_errno = errno;
			s->cafebabe_errno = CAFEBABE_ERROR_ERRNO;
			goto out;
		}

		if (cafebabe_constant_info_class_init(cp->info.class, s)) {
			free(cp->info.class);
			goto out;
		}

		break;
	case CAFEBABE_CONSTANT_TAG_STRING:
		break;
	case CAFEBABE_CONSTANT_TAG_FIELD_REF:
		cp->info.field_ref = malloc(sizeof(*cp->info.field_ref));
		if (!cp->info.field_ref) {
			s->syscall_errno = errno;
			s->cafebabe_errno = CAFEBABE_ERROR_ERRNO;
		}

		if (cafebabe_constant_info_field_ref_init(
			cp->info.field_ref, s))
		{
			free(cp->info.field_ref);
			goto out;
		}

		break;
	case CAFEBABE_CONSTANT_TAG_METHOD_REF:
		cp->info.method_ref = malloc(sizeof(*cp->info.method_ref));
		if (!cp->info.method_ref) {
			s->syscall_errno = errno;
			s->cafebabe_errno = CAFEBABE_ERROR_ERRNO;
			goto out;
		}

		if (cafebabe_constant_info_method_ref_init(
			cp->info.method_ref, s))
		{
			free(cp->info.method_ref);
			goto out;
		}

		break;
	case CAFEBABE_CONSTANT_TAG_INTERFACE_METHOD_REF:
		break;
	case CAFEBABE_CONSTANT_TAG_NAME_AND_TYPE:
		cp->info.name_and_type = malloc(
			sizeof(*cp->info.name_and_type));
		if (!cp->info.name_and_type) {
			s->syscall_errno = errno;
			s->cafebabe_errno = CAFEBABE_ERROR_ERRNO;
			goto out;
		}

		if (cafebabe_constant_info_name_and_type_init(
			cp->info.name_and_type, s))
		{
			free(cp->info.name_and_type);
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
}
