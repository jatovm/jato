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

#include "cafebabe/access.h"
#include "cafebabe/class.h"
#include "cafebabe/constant_pool.h"
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
	if (cafebabe_class_constant_get_class(&class, class.this_class,
		&this_class))
	{
		fprintf(stderr, "error: %s: invalid this_class index\n",
			filename);
		goto out_stream;
	}

	struct cafebabe_constant_info_utf8 *this_class_name;
	if (cafebabe_class_constant_get_utf8(&class, this_class->name_index,
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
	if (cafebabe_class_constant_get_class(&class, class.super_class,
		&super_class))
	{
		fprintf(stderr, "error: %s: invalid super_class index\n",
			filename);
		goto out_stream;
	}

	struct cafebabe_constant_info_utf8 *super_class_name;
	if (cafebabe_class_constant_get_utf8(&class, super_class->name_index,
		&super_class_name))
	{
		fprintf(stderr, "error: %s: invalid super_class name index\n",
			filename);
		goto out_stream;
	}

	printf("super_class: %.*s\n",
		super_class_name->length, super_class_name->bytes);

	/* Try to output the class declaration */
	printf("declaration: ");
	if (class.access_flags & CAFEBABE_ACCESS_PUBLIC)
		printf("public ");
	if (class.access_flags & CAFEBABE_ACCESS_FINAL)
		printf("final ");
	if (class.access_flags & CAFEBABE_ACCESS_ABSTRACT)
		printf("abstract ");
	printf("%s ", class.access_flags & CAFEBABE_ACCESS_INTERFACE
		? "interface" : "class");
	printf("%.*s ", this_class_name->length, this_class_name->bytes);
	printf("extends %.*s\n",
		super_class_name->length, super_class_name->bytes);

	cafebabe_class_deinit(&class);
	cafebabe_stream_close(&stream);

	return EXIT_SUCCESS;

out_stream:
	cafebabe_stream_close(&stream);
out:
	return EXIT_FAILURE;
}
