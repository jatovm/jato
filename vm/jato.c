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
#include "jit/text.h"

#include "vm/class.h"
#include "vm/classloader.h"
#include "vm/fault-inject.h"
#include "vm/preload.h"
#include "vm/itable.h"
#include "vm/method.h"
#include "vm/natives.h"
#include "vm/object.h"
#include "vm/signal.h"
#include "vm/stack-trace.h"
#include "vm/static.h"
#include "vm/system.h"
#include "vm/vm.h"

static bool perf_enabled;
char *exe_name;

static struct vm_object *__vm_native native_vmstackwalker_getclasscontext(void)
{
	struct vm_object *res;

	NOT_IMPLEMENTED;

	res = vm_object_alloc_array(vm_java_lang_Class, 1);

	return res;
}

static void __vm_native native_vmsystemproperties_preinit(struct vm_object *p)
{
	struct vm_object *key = vm_object_alloc_string_from_c("java.vm.name");
	struct vm_object *value = vm_object_alloc_string_from_c("jato");

	struct vm_object *(*trampoline)(struct vm_object *,
		struct vm_object *, struct vm_object *);

	trampoline
		= vm_method_trampoline_ptr(vm_java_util_Properties_setProperty);

	trampoline(p, key, value);

	key = vm_object_alloc_string_from_c("java.io.tmpdir");
	value = vm_object_alloc_string_from_c("/tmp");
	trampoline(p, key, value);
}

static void __vm_native native_vmruntime_exit(int status)
{
	/* XXX: exit gracefully */
	exit(status);
}

static void __vm_native native_vmruntime_println(struct vm_object *message)
{
	char *cstr = vm_string_to_cstr(message);

	if (cstr)
		printf("%s\n", cstr);

	free(cstr);
}

static void __vm_native
native_vmsystem_arraycopy(struct vm_object *src, int src_start,
			  struct vm_object *dest, int dest_start, int len)
{
	const struct vm_class *src_elem_class;
	const struct vm_class *dest_elem_class;
	enum vm_type elem_type;
	int elem_size;

	if (!src || !dest || !src->class || !dest->class) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		goto throw;
	}

	if (!vm_class_is_array_class(src->class) ||
	    !vm_class_is_array_class(dest->class)) {
		signal_new_exception(vm_java_lang_ArrayStoreException, NULL);
		goto throw;
	}

	src_elem_class = vm_class_get_array_element_class(src->class);
	dest_elem_class = vm_class_get_array_element_class(dest->class);
	if (!src_elem_class || !dest_elem_class) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		goto throw;
	}

	elem_type = vm_class_get_storage_vmtype(src_elem_class);
	if (elem_type != vm_class_get_storage_vmtype(dest_elem_class)) {
		NOT_IMPLEMENTED;
		return;
	}

	if (len < 0 ||
	    src_start < 0 || src_start + len > src->array_length ||
	    dest_start < 0 || dest_start + len > dest->array_length) {
		signal_new_exception(
			vm_java_lang_ArrayIndexOutOfBoundsException, NULL);
		goto throw;
	}

	elem_size = get_vmtype_size(elem_type);
	memmove(dest->fields + dest_start * elem_size,
		src->fields + src_start * elem_size,
		len * elem_size);

	return;
 throw:
	throw_from_native(sizeof(int) * 3 + sizeof(struct vm_object*) * 2);
}

static int32_t __vm_native native_vmsystem_identityhashcode(struct vm_object *obj)
{
	return (int32_t) obj;
}

static struct vm_object * __vm_native
native_vmobject_getclass(struct vm_object *object)
{
	if (!object || !object->class) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		throw_from_native(sizeof object);
	}

	return object->class->object;
}

static struct vm_object * __vm_native
native_vmclass_getname(struct vm_object *object)
{
	struct vm_class *class;

	class = (struct vm_class*)field_get_object(object,
						   vm_java_lang_Class_vmdata);
	assert(class != NULL);

	return vm_object_alloc_string_from_c(class->name);
}

static struct vm_object * __vm_native
native_vmclassloader_getprimitiveclass(int type)
{
	char primitive_class_name[] = { "X" };
	struct vm_class *class;

	primitive_class_name[0] = (char)type;

	class = classloader_load_primitive(primitive_class_name);
	if (!class)
		return NULL;

	vm_class_ensure_init(class);
	if (exception_occurred())
		throw_from_native(sizeof(int));

	return class->object;
}

static struct vm_native natives[] = {
	DEFINE_NATIVE("gnu/classpath/VMStackWalker", "getClassContext", &native_vmstackwalker_getclasscontext),
	DEFINE_NATIVE("gnu/classpath/VMSystemProperties", "preInit", &native_vmsystemproperties_preinit),
	DEFINE_NATIVE("jato/internal/VM", "exit", &native_vmruntime_exit),
	DEFINE_NATIVE("jato/internal/VM", "println", &native_vmruntime_println),
	DEFINE_NATIVE("java/lang/VMClass", "getName", &native_vmclass_getname),
	DEFINE_NATIVE("java/lang/VMClassLoader", "getPrimitiveClass", &native_vmclassloader_getprimitiveclass),
	DEFINE_NATIVE("java/lang/VMObject", "getClass", &native_vmobject_getclass),
	DEFINE_NATIVE("java/lang/VMRuntime", "exit", &native_vmruntime_exit),
	DEFINE_NATIVE("java/lang/VMSystem", "arraycopy", &native_vmsystem_arraycopy),
	DEFINE_NATIVE("java/lang/VMSystem", "identityHashCode", &native_vmsystem_identityhashcode),
	DEFINE_NATIVE("java/lang/VMThrowable", "fillInStackTrace", &native_vmthrowable_fill_in_stack_trace),
	DEFINE_NATIVE("java/lang/VMThrowable", "getStackTrace", &native_vmthrowable_get_stack_trace),
	DEFINE_NATIVE("jato/internal/VM", "enableFault", &native_vm_enable_fault),
	DEFINE_NATIVE("jato/internal/VM", "disableFault", &native_vm_disable_fault),
};

static void jit_init_natives(void)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(natives); i++)
		vm_register_native(&natives[i]);
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
		} else if (!strcmp(argv[i], "-Xtrace:classloader")) {
			opt_trace_classloader = true;
		} else if (!strcmp(argv[i], "-Xtrace:invoke")) {
			opt_trace_invoke = true;
		} else if (!strcmp(argv[i], "-Xtrace:exceptions")) {
			opt_trace_exceptions = true;
		} else if (!strcmp(argv[i], "-Xtrace:invoke-verbose")) {
			opt_trace_invoke = true;
			opt_trace_invoke_verbose = true;
		} else if (!strcmp(argv[i], "-Xtrace:itable")) {
			opt_trace_itable = true;
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
		} else if (!strcmp(argv[i], "-Xperf")) {
			perf_enabled = true;
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

	jit_text_init();

	if (perf_enabled)
		perf_map_open();

	setup_signal_handlers();
	init_cu_mapping();
	init_exceptions();

	jit_init_natives();

	static_fixup_init();

	/* Search $CLASSPATH last. */
	char *classpath = getenv("CLASSPATH");
	if (classpath)
		classloader_add_to_classpath(classpath);

	if (preload_vm_classes()) {
		NOT_IMPLEMENTED;
		exit(EXIT_FAILURE);
	}

	init_stack_trace_printing();

	struct vm_class *vmc = classloader_load(classname);
	if (!vmc) {
		fprintf(stderr, "error: %s: could not load\n", classname);
		goto out;
	}

	if (vm_class_ensure_init(vmc)) {
		fprintf(stderr, "error: %s: couldn't initialize\n", classname);
		goto out;
	}

	struct vm_method *vmm = vm_class_get_method_recursive(vmc,
		"main", "([Ljava/lang/String;)V");
	if (!vmm) {
		fprintf(stderr, "error: %s: no main method\n", classname);
		goto out;
	}

	if (!vm_method_is_static(vmm)) {
		fprintf(stderr, "error: %s: main method not static\n",
			classname);
		goto out;
	}

	bottom_stack_frame = __builtin_frame_address(0);

	void (*main_method_trampoline)(void)
		= vm_method_trampoline_ptr(vmm);
	main_method_trampoline();

	if (exception_occurred()) {
		struct vm_object *exception;

		exception = exception_occurred();
		clear_exception();

		vm_print_exception(exception);
		goto out;
	}
	status = EXIT_SUCCESS;
out:
	return status;
}
