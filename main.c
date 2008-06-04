#include <stdio.h>
#include <stdlib.h>

#include "cafebabe/class.h"
#include "cafebabe/constant_pool.h"
#include "cafebabe/error.h"
#include "cafebabe/stream.h"

static int
constant_pool_index_invalid(struct cafebabe_class *c, uint16_t index)
{
	if (index < 1 || index >= c->constant_pool_count)
		return 1;

	return 0;
}

static int
get_constant_tag_class(struct cafebabe_class *c, uint16_t index,
	struct cafebabe_constant_info_class **r)
{
	if (constant_pool_index_invalid(c, index))
		return 1;

	struct cafebabe_constant_pool *pool = &c->constant_pool[index];
	if (pool->tag != CAFEBABE_CONSTANT_TAG_CLASS)
		return 1;

	*r = pool->info.class;
	return 0;
}

static int
get_constant_tag_utf8(struct cafebabe_class *c, uint16_t index,
	struct cafebabe_constant_info_utf8 **r)
{
	if (constant_pool_index_invalid(c, index))
		return 1;

	struct cafebabe_constant_pool *pool = &c->constant_pool[index];
	if (pool->tag != CAFEBABE_CONSTANT_TAG_UTF8)
		return 1;

	*r = pool->info.utf8;
	return 0;
}

int
main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s class-file\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	const char *filename = argv[1];

	struct cafebabe_stream stream;
	if (cafebabe_stream_open(&stream, filename)) {
		fprintf(stderr, "error: %s: %s\n", filename,
			cafebabe_stream_error(&stream));
		goto out;
	}

	struct cafebabe_class class;
	if (cafebabe_class_init(&class, &stream)) {
		fprintf(stderr, "error: %s:%d/%d: %s\n", filename,
			stream.virtual_i, stream.virtual_n,
			cafebabe_stream_error(&stream));
		goto out_stream;
	}

	printf("magic: %08x\n", class.magic);
	printf("version: %d.%d\n", class.major_version, class.minor_version);

	/* Look up class name */
	struct cafebabe_constant_info_class *this_class;
	if (get_constant_tag_class(&class, class.this_class, &this_class)) {
		fprintf(stderr, "error: %s: invalid this_class index\n",
			filename);
		goto out_stream;
	}

	struct cafebabe_constant_info_utf8 *this_class_name;
	if (get_constant_tag_utf8(&class, this_class->name_index,
		&this_class_name))
	{
		fprintf(stderr, "error: %s: invalid this_class name index\n",
			filename);
		goto out_stream;
	}

	printf("this_class: %.*s\n",
		this_class_name->length, this_class_name->bytes);

	/* Look up superclass name */
	struct cafebabe_constant_info_class *super_class;
	if (get_constant_tag_class(&class, class.super_class, &super_class)) {
		fprintf(stderr, "error: %s: invalid super_class index\n",
			filename);
		goto out_stream;
	}

	struct cafebabe_constant_info_utf8 *super_class_name;
	if (get_constant_tag_utf8(&class, super_class->name_index,
		&super_class_name))
	{
		fprintf(stderr, "error: %s: invalid super_class name index\n",
			filename);
		goto out_stream;
	}

	printf("super_class: %.*s\n",
		super_class_name->length, super_class_name->bytes);

	cafebabe_class_deinit(&class);
	cafebabe_stream_close(&stream);

	return EXIT_SUCCESS;

out_stream:
	cafebabe_stream_close(&stream);
out:
	return EXIT_FAILURE;
}
