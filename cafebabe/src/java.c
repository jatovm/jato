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

#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cafebabe/access.h"
#include "cafebabe/attribute_array.h"
#include "cafebabe/attribute_info.h"
#include "cafebabe/class.h"
#include "cafebabe/code_attribute.h"
#include "cafebabe/constant_pool.h"
#include "cafebabe/error.h"
#include "cafebabe/field_info.h"
#include "cafebabe/method_info.h"
#include "cafebabe/stream.h"

int
main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s CLASS\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	const char *classname = argv[1];
	char *filename;
	if (asprintf(&filename, "%s.class", classname) == -1) {
		fprintf(stderr, "error: %s\n", strerror(errno));
		goto out;
	}

	struct cafebabe_stream stream;
	if (cafebabe_stream_open(&stream, filename)) {
		fprintf(stderr, "error: %s: %s\n", filename,
			cafebabe_stream_error(&stream));
		goto out_filename;
	}

	struct cafebabe_class class;
	if (cafebabe_class_init(&class, &stream)) {
		fprintf(stderr, "error: %s:%d/%d: %s\n", filename,
			stream.virtual_i, stream.virtual_n,
			cafebabe_stream_error(&stream));
		goto out_stream;
	}

	unsigned int main_method_index;
	if (cafebabe_class_get_method(&class,
		"main", "([Ljava/lang/String;)V", &main_method_index))
	{
		fprintf(stderr, "error: %s: no main method\n", classname);
		goto out_class;
	}

	struct cafebabe_method_info *main_method
		= &class.methods[main_method_index];

	unsigned int main_code_index = 0;
	if (cafebabe_attribute_array_get(&main_method->attributes,
		"Code", &class, &main_code_index))
	{
		fprintf(stderr, "error: %s: no 'Class' attribute for main "
			"method\n", classname);
		goto out_class;
	}

	struct cafebabe_attribute_info *main_code_attribute_info
		= &main_method->attributes.array[main_code_index];

	struct cafebabe_stream main_code_attribute_stream;
	cafebabe_stream_open_buffer(&main_code_attribute_stream,
		main_code_attribute_info->info,
		main_code_attribute_info->attribute_length);

	struct cafebabe_code_attribute main_code_attribute;
	if (cafebabe_code_attribute_init(&main_code_attribute,
		&main_code_attribute_stream))
	{
		fprintf(stderr, "error: %s:%s:%d/%d: %s\n",
			filename,
			"main",
			main_code_attribute_stream.virtual_i,
			main_code_attribute_stream.virtual_n,
			cafebabe_stream_error(&main_code_attribute_stream));
		exit(1);
	}

	cafebabe_stream_close_buffer(&main_code_attribute_stream);

	printf("main:\n");
	printf("\tmax_stack = %d\n", main_code_attribute.max_stack);
	printf("\tmax_locals = %d\n", main_code_attribute.max_locals);
	printf("\tcode_length = %d\n", main_code_attribute.code_length);

	cafebabe_code_attribute_deinit(&main_code_attribute);

	cafebabe_class_deinit(&class);
	cafebabe_stream_close(&stream);
	free(filename);

	return EXIT_SUCCESS;

out_class:
	cafebabe_class_deinit(&class);
out_stream:
	cafebabe_stream_close(&stream);
out_filename:
	free(filename);
out:
	return EXIT_FAILURE;
}
