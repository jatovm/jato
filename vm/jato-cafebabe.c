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

#include <ctype.h>
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
#include "vm/classloader.h"
#include "vm/method.h"
#include "vm/natives.h"
#include "vm/object.h"
#include "vm/signal.h"
#include "vm/vm.h"

char *exe_name;

static void native_vmruntime_exit(int status)
{
	/* XXX: exit gracefully */
	exit(status);
}

static void native_vmruntime_println(struct vm_object *message)
{
	struct vm_field *offset_field
		= vm_class_get_field(message->class, "offset", "I");
	if (!offset_field) {
		NOT_IMPLEMENTED;
		return;
	}

	struct vm_field *count_field
		= vm_class_get_field(message->class, "count", "I");
	if (!count_field) {
		NOT_IMPLEMENTED;
		return;
	}

	struct vm_field *value_field
		= vm_class_get_field(message->class, "value", "[C");
	if (!value_field) {
		NOT_IMPLEMENTED;
		return;
	}

	int32_t offset = *(int32_t *) &message->fields[offset_field->offset];
	int32_t count = *(int32_t *) &message->fields[count_field->offset];
	struct vm_object *array_object
		= *(struct vm_object **) &message->fields[value_field->offset];
	int16_t *array = (int16_t *) array_object->fields;

	for (int32_t i = 0; i < count; ++i) {
		int16_t ch = array[offset + i];

		if (ch < 128 && isprint(ch))
			printf("%c", ch);
		else
			printf("<%d>", ch);
	}

	printf("\n");
}

static void native_vmsystem_arraycopy(struct vm_object *src, int src_start,
	struct vm_object *dest, int dest_start, int len)
{
	NOT_IMPLEMENTED;
}

/*
 * This stub is needed by java.lang.VMThrowable constructor to work. It should
 * return java.lang.VMState instance, or null in which case no stack trace will
 * be printed by printStackTrace() method.
 */
static struct vm_object *
native_vmthrowable_fill_in_stack_trace(struct vm_object *message)
{
	NOT_IMPLEMENTED;

	return NULL;
}

static void jit_init_natives(void)
{
	vm_register_native("jato/internal/VM", "exit",
		&native_vmruntime_exit);
	vm_register_native("jato/internal/VM", "println",
		&native_vmruntime_println);
	vm_register_native("java/lang/VMRuntime", "exit",
		&native_vmruntime_exit);
	vm_register_native("java/lang/VMSystem", "arraycopy",
		&native_vmsystem_arraycopy);
	vm_register_native("java/lang/VMThrowable", "fillInStackTrace",
		&native_vmthrowable_fill_in_stack_trace);
}

static void native_Test_printf(int x)
{
	printf("%d\n", x);
}

int
main(int argc, char *argv[])
{
	exe_name = argv[0];

#ifndef NDEBUG
	/* Make stdout/stderr unbuffered; it really helps debugging! */
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
#endif

	if (argc != 2) {
		fprintf(stderr, "usage: %s CLASS\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	jit_init_natives();
	vm_register_native("Test", "printf", &native_Test_printf);

	const char *classname = argv[1];
	struct vm_class *vmc = classloader_load_and_init(classname);
	if (!vmc) {
		fprintf(stderr, "error: %s: could not load\n", classname);
		goto out;
	}

	struct vm_method *vmm = vm_class_get_method_recursive(vmc,
		"main", "([Ljava/lang/String;)V");
	if (!vmm) {
		fprintf(stderr, "error: %s: no main method\n", classname);
		goto out;
	}

	if (!vm_method_is_static(vmm)) {
		fprintf(stderr, "errror: %s: main method not static\n",
			classname);
		goto out;
	}

	void (*main_method_trampoline)(void)
		= vm_method_trampoline_ptr(vmm);
	main_method_trampoline();

	return EXIT_SUCCESS;

out:
	return EXIT_FAILURE;
}
