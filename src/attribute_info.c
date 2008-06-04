#include <stdio.h>
#include <stdlib.h>

#include "cafebabe/attribute_info.h"
#include "cafebabe/stream.h"

int
cafebabe_attribute_info_init(struct cafebabe_attribute_info *a,
	struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &a->attribute_name_index))
		goto out;

	if (cafebabe_stream_read_uint32(s, &a->attribute_length))
		goto out;

	a->info = malloc(a->attribute_length);
	if (!a->info)
		goto out;

	for (uint8_t i = 0; i < a->attribute_length; ++i) {
		if (cafebabe_stream_read_uint8(s, &a->info[i]))
			goto out_info;
	}

	return 0;

out_info:
	free(a->info);
out:
	return 1;
}

void
cafebabe_attribute_info_deinit(struct cafebabe_attribute_info *a)
{
	free(a->info);
}
