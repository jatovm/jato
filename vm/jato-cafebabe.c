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
#include "cafebabe/attribute_info.h"
#include "cafebabe/class.h"
#include "cafebabe/constant_pool.h"
#include "cafebabe/error.h"
#include "cafebabe/field_info.h"
#include "cafebabe/method_info.h"
#include "cafebabe/stream.h"

#include "jit/compiler.h"

#include "vm/class.h"
#include "vm/method.h"
#include "vm/natives.h"
#include "vm/signal.h"
#include "vm/vm.h"

char *exe_name;

static void vm_runtime_exit(int status)
{
	NOT_IMPLEMENTED;

	exit(status);
}

/*
 * This stub is needed by java.lang.VMThrowable constructor to work. It should
 * return java.lang.VMState instance, or null in which case no stack trace will
 * be printed by printStackTrace() method.
 */
static struct object *vm_fill_in_stack_trace(struct object *object)
{
	NOT_IMPLEMENTED;

	return NULL;
}

static void jit_init_natives(void)
{
	vm_register_native("java/lang/VMRuntime", "exit",
		vm_runtime_exit);
	vm_register_native("java/lang/VMThrowable", "fillInStackTrace",
		vm_fill_in_stack_trace);
}

int
main(int argc, char *argv[])
{
	exe_name = argv[0];

	if (argc != 2) {
		fprintf(stderr, "usage: %s CLASS\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	jit_init_natives();

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
		free(filename);
		goto out;
	}

	struct cafebabe_class class;
	if (cafebabe_class_init(&class, &stream)) {
		fprintf(stderr, "error: %s:%d/%d: %s\n", filename,
			stream.virtual_i, stream.virtual_n,
			cafebabe_stream_error(&stream));
		cafebabe_stream_close(&stream);
		free(filename);
		goto out;
	}

	cafebabe_stream_close(&stream);
	free(filename);

	struct vm_class vmc;
	vm_class_init(&vmc, &class);

	unsigned int main_method_index;
	if (cafebabe_class_get_method(&class,
		"main", "([Ljava/lang/String;)V", &main_method_index))
	{
		fprintf(stderr, "error: %s: no main method\n", classname);
		goto out_class;
	}

#if 0
	jit_prepare_method(main_method);
	java_main_fn main_method_trampoline;
	main_method_trampoline = method_trampoline_ptr(main_method);
	main_method_trampoline();
#endif

	cafebabe_class_deinit(&class);
	return EXIT_SUCCESS;

out_class:
	cafebabe_class_deinit(&class);
out:
	return EXIT_FAILURE;
}
