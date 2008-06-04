#include <stdio.h>
#include <stdlib.h>

#include "cafebabe/class.h"
#include "cafebabe/error.h"
#include "cafebabe/stream.h"

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
		exit(EXIT_FAILURE);
	}

	struct cafebabe_class class;
	if (cafebabe_class_init(&class, &stream)) {
		fprintf(stderr, "error: %s:%d/%d: %s\n", filename,
			stream.virtual_i, stream.virtual_n,
			cafebabe_stream_error(&stream));
		exit(EXIT_FAILURE);
	}

	cafebabe_class_deinit(&class);
	cafebabe_stream_close(&stream);

	return EXIT_SUCCESS;
}
