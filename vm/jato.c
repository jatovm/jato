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

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
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
#include "jit/cu-mapping.h"
#include "jit/exception.h"
#include "jit/perf-map.h"

#include "vm/class.h"
#include "vm/classloader.h"
#include "vm/java_lang.h"
#include "vm/method.h"
#include "vm/natives.h"
#include "vm/object.h"
#include "vm/signal.h"
#include "vm/stack-trace.h"
#include "vm/system.h"
#include "vm/vm.h"

char *exe_name;

static void __vm_native native_vmruntime_exit(int status)
{
	/* XXX: exit gracefully */
	exit(status);
}

static void __vm_native native_vmruntime_println(struct vm_object *message)
{
	int32_t offset = *(int32_t *)
		&message->fields[vm_java_lang_String_offset->offset];
	int32_t count = *(int32_t *)
		&message->fields[vm_java_lang_String_count->offset];
	struct vm_object *array_object = *(struct vm_object **)
		&message->fields[vm_java_lang_String_value->offset];

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

static void __vm_native native_vmsystem_arraycopy(struct vm_object *src, int src_start,
	struct vm_object *dest, int dest_start, int len)
{
	NOT_IMPLEMENTED;
}

/*
 * This stub is needed by java.lang.VMThrowable constructor to work. It should
 * return java.lang.VMState instance, or null in which case no stack trace will
 * be printed by printStackTrace() method.
 */
static struct vm_object * __vm_native
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

struct preload_entry {
	const char *name;
	struct vm_class **class;
};

struct vm_class *vm_java_lang_Object;
struct vm_class *vm_java_lang_Class;
struct vm_class *vm_java_lang_String;

static const struct preload_entry preload_entries[] = {
	{ "java/lang/Object",	&vm_java_lang_Object },
	{ "java/lang/Class",	&vm_java_lang_Class },
	{ "java/lang/String",	&vm_java_lang_String },
};

struct field_preload_entry {
	struct vm_class **class;
	const char *name;
	const char *type;
	struct vm_field **field;
};

struct vm_field *vm_java_lang_String_offset;
struct vm_field *vm_java_lang_String_count;
struct vm_field *vm_java_lang_String_value;

static const struct field_preload_entry field_preload_entries[] = {
	{ &vm_java_lang_String, "offset", "I",	&vm_java_lang_String_offset },
	{ &vm_java_lang_String, "count", "I",	&vm_java_lang_String_count },
	{ &vm_java_lang_String, "value", "[C",	&vm_java_lang_String_value },
};

static int preload_vm_classes(void)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(preload_entries); ++i) {
		const struct preload_entry *pe = &preload_entries[i];

		struct vm_class *class = classloader_load(pe->name);
		if (!class) {
			NOT_IMPLEMENTED;
			return 1;
		}

		*pe->class = class;
	}

	for (unsigned int i = 0; i < ARRAY_SIZE(preload_entries); ++i) {
		const struct preload_entry *pe = &preload_entries[i];

		if (vm_class_init_object(*pe->class)) {
			NOT_IMPLEMENTED;
			return 1;
		}
	}

	for (unsigned int i = 0; i < ARRAY_SIZE(field_preload_entries); ++i) {
		const struct field_preload_entry *pe
			= &field_preload_entries[i];

		struct vm_field *field = vm_class_get_field(*pe->class,
			pe->name, pe->type);
		if (!field) {
			NOT_IMPLEMENTED;
			return 1;
		}

		*pe->field = field;
	}

	return 0;
}

static void usage(FILE *f, int retval)
{
	fprintf(f, "usage: %s [options] class\n", exe_name);
	exit(retval);
}

int
main(int argc, char *argv[])
{
	int status = EXIT_FAILURE;

	exe_name = argv[0];

#ifndef NDEBUG
	/* Make stdout/stderr unbuffered; it really helps debugging! */
	setvbuf(stdout, NULL, _IONBF, 0);
	setvbuf(stderr, NULL, _IONBF, 0);
#endif

	/* We need to support at least this:
	 *  -classpath/-cp
	 *  -Xtrace:jit
	 *  -Xtrace:trampoline
	 *  -Xtrace:bytecode-offset
	 *  -Xtrace:asm
	 */

	const char *classname = NULL;

	for (int i = 1; i < argc; ++i) {
		if (!strcmp(argv[i], "-h") || !strcmp(argv[i], "-help")) {
			usage(stdout, EXIT_SUCCESS);
		} else if (!strcmp(argv[i], "-classpath")
			|| !strcmp(argv[i], "-cp"))
		{
			if (++i >= argc) {
				NOT_IMPLEMENTED;
				break;
			}

			if (classloader_add_to_classpath(argv[i]))
				NOT_IMPLEMENTED;
		} else if (!strcmp(argv[i], "-Xtrace:asm")) {
			opt_trace_method = true;
			opt_trace_machine_code = true;
		} else if (!strcmp(argv[i], "-Xtrace:bytecode-offset")) {
			opt_trace_bytecode_offset = true;
		} else if (!strcmp(argv[i], "-Xtrace:jit")) {
			opt_trace_method = true;
			opt_trace_cfg = true;
			opt_trace_tree_ir = true;
			opt_trace_lir = true;
			opt_trace_liveness = true;
			opt_trace_regalloc = true;
			opt_trace_machine_code = true;
			opt_trace_magic_trampoline = true;
			opt_trace_bytecode_offset = true;
		} else if (!strcmp(argv[i], "-Xtrace:trampoline")) {
			opt_trace_magic_trampoline = true;
		} else {
			if (argv[i][0] == '-')
				usage(stderr, EXIT_FAILURE);

			if (classname)
				usage(stderr, EXIT_FAILURE);

			classname = argv[i];
		}
	}

	if (!classname)
		usage(stderr, EXIT_FAILURE);

	perf_map_open();

	setup_signal_handlers();
	init_cu_mapping();
	init_exceptions();

	jit_init_natives();

	/* Search $CLASSPATH last. */
	char *classpath = getenv("CLASSPATH");
	if (classpath)
		classloader_add_to_classpath(classpath);

	if (preload_vm_classes()) {
		NOT_IMPLEMENTED;
		exit(EXIT_FAILURE);
	}

	init_stack_trace_printing();

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

	status = EXIT_SUCCESS;

	bottom_stack_frame = __builtin_frame_address(0);

	void (*main_method_trampoline)(void)
		= vm_method_trampoline_ptr(vmm);
	main_method_trampoline();

out:
	return status;
}
