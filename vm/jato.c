/*
 * jato - Java JIT compiler and VM
 * Copyright (C) 2009  Vegard Nossum <vegardno@ifi.uio.no>
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

#include <ctype.h>
#include <errno.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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
#include "vm/jni.h"
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
	struct stack_trace_elem st_elem;
	struct compilation_unit *cu;
	struct vm_class *class;
	struct vm_object *res;

	if (init_stack_trace_elem(&st_elem))
		return NULL;

	cu = jit_lookup_cu(st_elem.addr);
	if (!cu) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	class = cu->method->class;

	res = vm_object_alloc_array(vm_java_lang_Class, 1);
	array_set_field_ptr(res, 0, class->object);

	return res;
}

static void vm_properties_set_property(struct vm_object *p,
				       const char *key, const char *value)
{
	struct vm_object *(*trampoline)(struct vm_object *,
					struct vm_object *, struct vm_object *);

	trampoline
		= vm_method_trampoline_ptr(vm_java_util_Properties_setProperty);

	struct vm_object *key_obj = vm_object_alloc_string_from_c(key);
	struct vm_object *value_obj = vm_object_alloc_string_from_c(value);

	trampoline(p, key_obj, value_obj);
}

static void __vm_native native_vmsystemproperties_preinit(struct vm_object *p)
{
	struct system_properties_entry {
		const char *key;
		const char *value;
	};

	static const struct system_properties_entry system_properties[] = {
		{ "java.vm.name", "jato" },
		{ "java.io.tmpdir", "/tmp" },
		{ "file.separator", "/" },
		{ "path.separator", ":" },
		{ "line.separator", "\n" },
	};

	vm_properties_set_property(p, "java.library.path",
				   getenv("LD_LIBRARY_PATH"));

	for (unsigned int i = 0; i < ARRAY_SIZE(system_properties); ++i) {
		const struct system_properties_entry *e = &system_properties[i];

		vm_properties_set_property(p, e->key, e->value);
	}
}

static void __vm_native native_vmruntime_exit(int status)
{
	/* XXX: exit gracefully */
	exit(status);
}

static struct vm_object *__vm_native
native_vmruntime_maplibraryname(struct vm_object *name)
{
	struct vm_object *result;
	char *str;

	if (!name) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		throw_from_native(sizeof(struct vm_object));
		return NULL;
	}

	str = vm_string_to_cstr(name);
	if (!str) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	char *result_str = NULL;

	if (asprintf(&result_str, "lib%s.so", str) < 0)
		die("asprintf");

	free(str);

	if (!result_str) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	result = vm_object_alloc_string_from_c(result_str);
	free(result_str);

	return result;
}

static int __vm_native
native_vmruntime_native_load(struct vm_object *name,
			     struct vm_object *classloader)
{
	char *name_str;
	int result;

	if (classloader != NULL) {
		NOT_IMPLEMENTED;
		return 0;
	}

	if (!name) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		throw_from_native(sizeof(struct vm_object) * 2);
		return 0;
	}

	name_str = vm_string_to_cstr(name);
	if (!name_str) {
		NOT_IMPLEMENTED;
		return 0;
	}

	result  = vm_jni_load_object(name_str);
	free(name_str);

	return result == 0;
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
native_vmobject_clone(struct vm_object *object)
{
	if (!object) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		throw_from_native(sizeof object);
		return NULL;
	}

	return vm_object_clone(object);
}

static struct vm_object * __vm_native
native_vmobject_getclass(struct vm_object *object)
{
	if (!object) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		throw_from_native(sizeof object);
		return NULL;
	}

	assert(object->class);

	return object->class->object;
}

static struct vm_object * __vm_native
native_vmclass_getname(struct vm_object *object)
{
	struct vm_class *class;

	class = vm_class_get_class_from_class_object(object);
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

static int __vm_native
native_vmfile_is_directory(struct vm_object *dirpath)
{
	char *dirpath_str;
	struct stat buf;

	dirpath_str = vm_string_to_cstr(dirpath);
	if (!dirpath_str)
		return false;

	if (stat(dirpath_str, &buf)) {
		free(dirpath_str);
		return false;
	}

	free(dirpath_str);

	return S_ISDIR(buf.st_mode);
}

static struct vm_native natives[] = {
	DEFINE_NATIVE("gnu/classpath/VMStackWalker", "getClassContext", &native_vmstackwalker_getclasscontext),
	DEFINE_NATIVE("gnu/classpath/VMSystemProperties", "preInit", &native_vmsystemproperties_preinit),
	DEFINE_NATIVE("jato/internal/VM", "exit", &native_vmruntime_exit),
	DEFINE_NATIVE("jato/internal/VM", "println", &native_vmruntime_println),
	DEFINE_NATIVE("java/lang/VMClass", "getName", &native_vmclass_getname),
	DEFINE_NATIVE("java/lang/VMClassLoader", "getPrimitiveClass", &native_vmclassloader_getprimitiveclass),
	DEFINE_NATIVE("java/io/VMFile", "isDirectory", &native_vmfile_is_directory),
	DEFINE_NATIVE("java/lang/VMObject", "clone", &native_vmobject_clone),
	DEFINE_NATIVE("java/lang/VMObject", "getClass", &native_vmobject_getclass),
	DEFINE_NATIVE("java/lang/VMRuntime", "exit", &native_vmruntime_exit),
	DEFINE_NATIVE("java/lang/VMRuntime", "mapLibraryName", &native_vmruntime_maplibraryname),
	DEFINE_NATIVE("java/lang/VMRuntime", "nativeLoad", &native_vmruntime_native_load),
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

static void handle_help(void)
{
	usage(stdout, EXIT_SUCCESS);
}

static void handle_classpath(const char *arg)
{
	classloader_add_to_classpath(arg);
}

static char *classname;

static void handle_perf(void)
{
	perf_enabled = true;
}

static void handle_trace_asm(void)
{
	opt_trace_method = true;
	opt_trace_machine_code = true;
}

static void handle_trace_bytecode_offset(void)
{
	opt_trace_bytecode_offset = true;
}

static void handle_trace_classloader(void)
{
	opt_trace_classloader = true;
}

static void handle_trace_exceptions(void)
{
	opt_trace_exceptions = true;
}

static void handle_trace_invoke(void)
{
	opt_trace_invoke = true;
}

static void handle_trace_invoke_verbose(void)
{
	opt_trace_invoke = true;
	opt_trace_invoke_verbose = true;
}

static void handle_trace_itable(void)
{
	opt_trace_itable = true;
}

static void handle_trace_jit(void)
{
	opt_trace_method = true;
	opt_trace_cfg = true;
	opt_trace_tree_ir = true;
	opt_trace_lir = true;
	opt_trace_liveness = true;
	opt_trace_regalloc = true;
	opt_trace_machine_code = true;
	opt_trace_magic_trampoline = true;
	opt_trace_bytecode_offset = true;
}

static void handle_trace_trampoline(void)
{
	opt_trace_magic_trampoline = true;
}

struct option {
	const char *name;

	bool arg;

	union {
		void (*func)(void);
		void (*func_arg)(const char *arg);
	} handler;
};

#define DEFINE_OPTION(_name, _handler) \
	{ .name = _name, .arg = false, .handler.func = _handler }

#define DEFINE_OPTION_ARG(_name, _handler) \
	{ .name = _name, .arg = true, .handler.func_arg = _handler }

const struct option options[] = {
	DEFINE_OPTION("h",		handle_help),
	DEFINE_OPTION("help",		handle_help),

	DEFINE_OPTION_ARG("classpath",	handle_classpath),
	DEFINE_OPTION_ARG("cp",		handle_classpath),

	DEFINE_OPTION("Xperf",			handle_perf),
	DEFINE_OPTION("Xtrace:asm",		handle_trace_asm),
	DEFINE_OPTION("Xtrace:bytecode-offset",	handle_trace_bytecode_offset),
	DEFINE_OPTION("Xtrace:classloader",	handle_trace_classloader),
	DEFINE_OPTION("Xtrace:exceptions",	handle_trace_exceptions),
	DEFINE_OPTION("Xtrace:invoke",		handle_trace_invoke),
	DEFINE_OPTION("Xtrace:invoke-verbose",	handle_trace_invoke_verbose),
	DEFINE_OPTION("Xtrace:itable",		handle_trace_itable),
	DEFINE_OPTION("Xtrace:jit",		handle_trace_jit),
	DEFINE_OPTION("Xtrace:trampoline",	handle_trace_trampoline),
};

static const struct option *get_option(const char *name)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(options); ++i) {
		if (!strcmp(name, options[i].name))
			return &options[i];
	}

	return NULL;
}

static void parse_options(int argc, char *argv[])
{
	int optind;

	for (optind = 1; optind < argc; ++optind) {
		if (argv[optind][0] != '-')
			break;

		const struct option *opt = get_option(argv[optind] + 1);
		if (!opt)
			usage(stderr, EXIT_FAILURE);

		if (!opt->arg) {
			opt->handler.func();
			continue;
		}

		/* We wanted an argument, but there was none */
		if (optind + 1 >= argc)
			usage(stderr, EXIT_FAILURE);

		opt->handler.func_arg(argv[++optind]);
	}

	if (optind < argc) {
		/* Can't specify both a jar and a class file */
		if (classname)
			usage(stderr, EXIT_FAILURE);

		classname = argv[optind++];
	}

	/* Should be no more options after this */
	if (optind < argc)
		usage(stderr, EXIT_FAILURE);

	/* Can't specify neither a jar and a class file */
	if (!classname)
		usage(stderr, EXIT_FAILURE);
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

	parse_options(argc, argv);

	init_vm_objects();

	jit_text_init();

	if (perf_enabled)
		perf_map_open();

	setup_signal_handlers();
	init_cu_mapping();
	init_exceptions();

	jit_init_natives();

	static_fixup_init();
	vm_jni_init();

	if (try_to_add_zip_to_classpath("/usr/local/classpath/share/classpath/glibj.zip") < 0)
		try_to_add_zip_to_classpath("/usr/share/classpath/glibj.zip");

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
