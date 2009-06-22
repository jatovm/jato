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

#include <stdio.h>
#include <stdlib.h>

#include "cafebabe/access.h"
#include "cafebabe/attribute_info.h"
#include "cafebabe/class.h"
#include "cafebabe/constant_pool.h"
#include "cafebabe/error.h"
#include "cafebabe/field_info.h"
#include "cafebabe/method_info.h"
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
	const struct cafebabe_constant_info_class *this_class;
	if (cafebabe_class_constant_get_class(&class, class.this_class,
		&this_class))
	{
		fprintf(stderr, "error: %s: invalid this_class index\n",
			filename);
		goto out_class;
	}

	const struct cafebabe_constant_info_utf8 *this_class_name;
	if (cafebabe_class_constant_get_utf8(&class, this_class->name_index,
		&this_class_name))
	{
		fprintf(stderr, "error: %s: invalid this_class name index\n",
			filename);
		goto out_class;
	}

	printf("this_class: %.*s\n",
		this_class_name->length, this_class_name->bytes);

	/* Look up superclass name */
	const struct cafebabe_constant_info_class *super_class;
	if (cafebabe_class_constant_get_class(&class, class.super_class,
		&super_class))
	{
		fprintf(stderr, "error: %s: invalid super_class index\n",
			filename);
		goto out_class;
	}

	const struct cafebabe_constant_info_utf8 *super_class_name;
	if (cafebabe_class_constant_get_utf8(&class, super_class->name_index,
		&super_class_name))
	{
		fprintf(stderr, "error: %s: invalid super_class name index\n",
			filename);
		goto out_class;
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

	for(uint16_t i = 0; i < class.fields_count; ++i) {
		const struct cafebabe_constant_info_utf8 *field_name;
		if (cafebabe_class_constant_get_utf8(&class,
			class.fields[i].name_index, &field_name))
		{
			fprintf(stderr,
				"error: %s: invalid field name index\n",
				filename);
			goto out_class;
		}

		printf("field: %.*s\n",
			field_name->length, field_name->bytes);
	}

	for(uint16_t i = 0; i < class.methods_count; ++i) {
		const struct cafebabe_constant_info_utf8 *method_name;
		if (cafebabe_class_constant_get_utf8(&class,
			class.methods[i].name_index, &method_name))
		{
			fprintf(stderr,
				"error: %s: invalid method name index\n",
				filename);
			goto out_class;
		}

		printf("method: %.*s\n",
			method_name->length, method_name->bytes);
	}

	cafebabe_class_deinit(&class);
	cafebabe_stream_close(&stream);

	return EXIT_SUCCESS;

out_class:
	cafebabe_class_deinit(&class);
out_stream:
	cafebabe_stream_close(&stream);
out:
	return EXIT_FAILURE;
}
