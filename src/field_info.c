#include <stdint.h>
#include <stdlib.h>

#include "cafebabe/attribute_info.h"
#include "cafebabe/field_info.h"
#include "cafebabe/stream.h"

int
cafebabe_field_info_init(struct cafebabe_field_info *f,
	struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &f->access_flags))
		goto out;

	if (cafebabe_stream_read_uint16(s, &f->name_index))
		goto out;

	if (cafebabe_stream_read_uint16(s, &f->descriptor_index))
		goto out;

	if (cafebabe_stream_read_uint16(s, &f->attributes_count))
		goto out;

	f->attributes = malloc(sizeof(*f->attributes) * f->attributes_count);
	if (!f->attributes)
		goto out;

	uint16_t attributes_i;
	for (uint16_t i = 0; i < f->attributes_count; ++i) {
		if (cafebabe_attribute_info_init(&f->attributes[i], s)) {
			attributes_i = i;
			goto out_attributes_init;
		}
	}
	attributes_i = f->attributes_count;

	return 0;

out_attributes_init:
	for (uint16_t i = 0; i < attributes_i; ++i)
		cafebabe_attribute_info_deinit(&f->attributes[i]);
	free(f->attributes);
out:
	return 1;
}

void
cafebabe_field_info_deinit(struct cafebabe_field_info *f)
{
	for (uint16_t i = 0; i < f->attributes_count; ++i)
		cafebabe_attribute_info_deinit(&f->attributes[i]);
	free(f->attributes);
}
