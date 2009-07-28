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
#include <time.h>
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

#include "lib/list.h"

#include "vm/call.h"
#include "vm/class.h"
#include "vm/classloader.h"
#include "vm/fault-inject.h"
#include "vm/preload.h"
#include "vm/itable.h"
#include "vm/jar.h"
#include "vm/jni.h"
#include "vm/method.h"
#include "vm/natives.h"
#include "vm/object.h"
#include "vm/signal.h"
#include "vm/stack-trace.h"
#include "vm/static.h"
#include "vm/system.h"
#include "vm/thread.h"
#include "vm/vm.h"

static bool perf_enabled;
char *exe_name;

static struct vm_object *native_vmstackwalker_getclasscontext(void)
{
	struct stack_trace_elem st_elem;
	struct compilation_unit *cu;
	struct vm_class *class;
	struct vm_object *res;

	init_stack_trace_elem_current(&st_elem);

	if (stack_trace_elem_next_java(&st_elem))
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

struct system_properties_entry {
	char *key;
	char *value;
	struct list_head list_node;
};

static struct list_head system_properties_list;

static void add_system_property(char *key, char *value)
{
	struct system_properties_entry *this;

	assert(key && value);

	list_for_each_entry(this, &system_properties_list, list_node) {
		if (strcmp(this->key, key) == 0) {
			free(this->value);
			free(key);

			this->value = value;
			return;
		}
	}

	struct system_properties_entry *ent = malloc(sizeof *ent);
	if (!ent)
		error("out of memory");

	ent->key = key;
	ent->value = value;
	list_add(&ent->list_node, &system_properties_list);
}

static void add_system_property_const(const char *key, const char *value)
{
	char *key_d;
	char *value_d;

	key_d = strdup(key);
	value_d = strdup(value);

	if (!key_d || !value_d)
		error("out of memory");

	add_system_property(key_d, value_d);
}

struct system_property {
	const char *key;
	const char *value;
};

static struct system_property system_properties[] = {
	{ "java.vm.name",	"jato"	},
	{ "java.io.tmpdir",	"/tmp"	},
	{ "file.separator",	"/"	},
	{ "path.separator",	":"	},
	{ "line.separator",	"\n"	},
};

/*
 * This sets default values of system properties. It should be called
 * before command line arguments are parsed because these properties
 * can be overriden by -Dkey=value option.
 */
static void init_system_properties(void)
{
	INIT_LIST_HEAD(&system_properties_list);

	for (unsigned int i = 0; i < ARRAY_SIZE(system_properties); i++) {
		struct system_property *p = &system_properties[i];

		add_system_property_const(p->key, p->value);
	}

	const char *s = getenv("LD_LIBRARY_PATH");
	if (!s)
		s = "/usr/lib/classpath/";

	add_system_property_const("java.library.path", s);
}

static void native_vmsystemproperties_preinit(struct vm_object *p)
{
	struct system_properties_entry *this, *t;

	list_for_each_entry(this, &system_properties_list, list_node)
		vm_properties_set_property(p, this->key, this->value);

	/* dealloc system properties list */
	list_for_each_entry_safe(this, t, &system_properties_list, list_node) {
		free(this->key);
		free(this->value);
		list_del(&this->list_node);
		free(this);
	}
}

static void native_vmruntime_exit(int status)
{
	/* XXX: exit gracefully */
	exit(status);
}

static void native_vmruntime_run_finalization_for_exit(void)
{
}

static struct vm_object *
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

static int
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

static void native_vmruntime_println(struct vm_object *message)
{
	if (!message) {
		printf("null\n");
		return;
	}

	char *cstr = vm_string_to_cstr(message);

	if (cstr)
		printf("%s\n", cstr);

	free(cstr);
}

static void
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

static int32_t hash_ptr_to_int32(void *p)
{
#ifdef CONFIG_64_BIT
	int64_t key = (int64_t) p;

	key = (~key) + (key << 18);
	key = key ^ (key >> 31);
	key = key * 21;
	key = key ^ (key >> 11);
	key = key + (key << 6);
	key = key ^ (key >> 22);

	return key;
#else
	return (int32_t) p;
#endif
}

static int32_t native_vmsystem_identityhashcode(struct vm_object *obj)
{
	return hash_ptr_to_int32(obj);
}

static struct vm_object *
native_vmobject_clone(struct vm_object *object)
{
	if (!object) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		throw_from_native(sizeof object);
		return NULL;
	}

	return vm_object_clone(object);
}

static struct vm_object *
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

static struct vm_object *
native_vmclass_getclassloader(struct vm_object *object)
{
	if (!object) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		throw_from_native(sizeof object);
		return NULL;
	}

	struct vm_class *class = vm_class_get_class_from_class_object(object);
	assert(class != NULL);

	NOT_IMPLEMENTED;
	return NULL;
}

static struct vm_object *
native_vmclass_forname(struct vm_object *name, jboolean initialize,
		       struct vm_object *loader)
{
	if (!name) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		goto throw;
	}

	const char *class_name = vm_string_to_cstr(name);
	if (!class_name) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	/* TODO: use @loader to load the class. */
	struct vm_class *class = classloader_load(class_name);
	if (!class) {
		signal_new_exception(vm_java_lang_ClassNotFoundException,
				     class_name);
		goto throw;
	}

	if (initialize) {
		vm_class_ensure_init(class);
		if (exception_occurred())
			goto throw;
	}

	return class->object;

 throw:
	throw_from_native(sizeof(name) + sizeof(initialize) + sizeof(loader));
	return NULL;
}

static struct vm_object *
native_vmclass_getname(struct vm_object *object)
{
	struct vm_class *class;

	class = vm_class_get_class_from_class_object(object);
	assert(class != NULL);

	return vm_object_alloc_string_from_c(class->name);
}

static int32_t
native_vmclass_isprimitive(struct vm_object *object)
{
	if (!object) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		throw_from_native(sizeof object);
		return 0;
	}

	struct vm_class *class = vm_class_get_class_from_class_object(object);
	assert(class != NULL);

	/* Note: This function returns a boolean value (despite the int32_t) */
	return class->kind == VM_CLASS_KIND_PRIMITIVE;
}

static struct vm_object *
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

static int
native_vmfile_is_directory(struct vm_object *dirpath)
{
	char *dirpath_str;
	struct stat buf;

	if (!dirpath) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		throw_from_native(sizeof(dirpath));
		return false;
	}

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

static void
native_vm_throw_null_pointer_exception(void)
{
	signal_new_exception(vm_java_lang_NullPointerException, NULL);
	throw_from_native(0);
}

static struct vm_object *native_vmthread_current_thread(void)
{
	return field_get_object(vm_get_exec_env()->thread->vmthread,
				vm_java_lang_VMThread_thread);
}

static void native_vmthread_start(struct vm_object *vmthread, jlong stacksize)
{
	vm_thread_start(vmthread);
}

static void native_vmobject_notify_all(struct vm_object *obj)
{
	vm_monitor_notify_all(&obj->monitor);

	if (exception_occurred())
		throw_from_native(sizeof(obj));
}

static jlong native_vmsystem_nano_time(void)
{
	struct timespec time;
	if (clock_gettime(CLOCK_MONOTONIC, &time)) {
		NOT_IMPLEMENTED;
		return 0;
	}

	return (unsigned long long)time.tv_sec * 1000000000ull +
		(unsigned long long)time.tv_nsec;
}

static struct vm_native natives[] = {
	DEFINE_NATIVE("gnu/classpath/VMStackWalker", "getClassContext", &native_vmstackwalker_getclasscontext),
	DEFINE_NATIVE("gnu/classpath/VMSystemProperties", "preInit", &native_vmsystemproperties_preinit),
	DEFINE_NATIVE("jato/internal/VM", "exit", &native_vmruntime_exit),
	DEFINE_NATIVE("jato/internal/VM", "println", &native_vmruntime_println),
	DEFINE_NATIVE("jato/internal/VM", "throwNullPointerException", &native_vm_throw_null_pointer_exception),
	DEFINE_NATIVE("java/lang/VMClass", "getClassLoader", &native_vmclass_getclassloader),
	DEFINE_NATIVE("java/lang/VMClass", "getName", &native_vmclass_getname),
	DEFINE_NATIVE("java/lang/VMClass", "forName", &native_vmclass_forname),
	DEFINE_NATIVE("java/lang/VMClass", "isPrimitive", &native_vmclass_isprimitive),
	DEFINE_NATIVE("java/lang/VMClassLoader", "getPrimitiveClass", &native_vmclassloader_getprimitiveclass),
	DEFINE_NATIVE("java/io/VMFile", "isDirectory", &native_vmfile_is_directory),
	DEFINE_NATIVE("java/lang/VMObject", "clone", &native_vmobject_clone),
	DEFINE_NATIVE("java/lang/VMObject", "notifyAll", &native_vmobject_notify_all),
	DEFINE_NATIVE("java/lang/VMObject", "getClass", &native_vmobject_getclass),
	DEFINE_NATIVE("java/lang/VMRuntime", "exit", &native_vmruntime_exit),
	DEFINE_NATIVE("java/lang/VMRuntime", "mapLibraryName", &native_vmruntime_maplibraryname),
	DEFINE_NATIVE("java/lang/VMRuntime", "nativeLoad", &native_vmruntime_native_load),
	DEFINE_NATIVE("java/lang/VMRuntime", "runFinalizationForExit", &native_vmruntime_run_finalization_for_exit),
	DEFINE_NATIVE("java/lang/VMSystem", "arraycopy", &native_vmsystem_arraycopy),
	DEFINE_NATIVE("java/lang/VMSystem", "identityHashCode", &native_vmsystem_identityhashcode),
	DEFINE_NATIVE("java/lang/VMSystem", "nanoTime", &native_vmsystem_nano_time),
	DEFINE_NATIVE("java/lang/VMThread", "currentThread", &native_vmthread_current_thread),
	DEFINE_NATIVE("java/lang/VMThread", "start", &native_vmthread_start),
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
static struct vm_jar *jar_file;

static void handle_jar(const char *arg)
{
	/* Can't specify more than one jar file */
	if (jar_file)
		usage(stderr, EXIT_FAILURE);

	jar_file = vm_jar_open(arg);
	if (!jar_file) {
		NOT_IMPLEMENTED;
		exit(EXIT_FAILURE);
	}

	const char *main_class = vm_jar_get_main_class(jar_file);
	if (!main_class) {
		NOT_IMPLEMENTED;
		exit(EXIT_FAILURE);
	}

	classname = strdup(main_class);
	if (!classname) {
		NOT_IMPLEMENTED;
		exit(EXIT_FAILURE);
	}

	/* XXX: Cheap solution. This can give funny results depending on where
	 * you put the -jar relative to the -classpath(s). Besides, we should
	 * save some memory and only open the zip file once. */
	classloader_add_to_classpath(arg);
}

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

static void handle_trace_bytecode(void)
{
	opt_trace_bytecode = true;
	opt_trace_method = true;
}

static void handle_trace_trampoline(void)
{
	opt_trace_magic_trampoline = true;
}

static void handle_define(const char *arg)
{
	char *str, *ptr, *key, *value;

	str = strdup(arg);
	if (!str)
		error("out of memory");

	key = strtok_r(str, "=", &ptr);
	value = strtok_r(NULL, "=", &ptr);

	if (!key || !*key)
		goto out;

	if (!value)
		value = "";

	key = strdup(key);
	value = strdup(value);

	if (!key || !value)
		error("out of memory");

	add_system_property(key, value);
 out:
	free(str);
}

struct option {
	const char *name;

	bool arg;
	bool arg_is_adjacent;

	union {
		void (*func)(void);
		void (*func_arg)(const char *arg);
	} handler;
};

#define DEFINE_OPTION(_name, _handler) \
	{ .name = _name, .arg = false, .handler.func = _handler }

#define DEFINE_OPTION_ARG(_name, _handler) \
	{ .name = _name, .arg = true, .arg_is_adjacent = false, .handler.func_arg = _handler }

#define DEFINE_OPTION_ADJACENT_ARG(_name, _handler) \
	{ .name = _name, .arg = true, .arg_is_adjacent = true, .handler.func_arg = _handler }

const struct option options[] = {
	DEFINE_OPTION("h",		handle_help),
	DEFINE_OPTION("help",		handle_help),

	DEFINE_OPTION_ADJACENT_ARG("D",	handle_define),

	DEFINE_OPTION_ARG("classpath",	handle_classpath),
	DEFINE_OPTION_ARG("cp",		handle_classpath),
	DEFINE_OPTION_ARG("jar",	handle_jar),

	DEFINE_OPTION("Xperf",			handle_perf),

	DEFINE_OPTION("Xtrace:asm",		handle_trace_asm),
	DEFINE_OPTION("Xtrace:bytecode",	handle_trace_bytecode),
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
		const struct option *opt = &options[i];

		if (opt->arg && opt->arg_is_adjacent &&
		    !strncmp(name, opt->name, strlen(opt->name)))
			return opt;

		if (!strcmp(name, opt->name))
			return opt;
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

		if (opt->arg_is_adjacent) {
			opt->handler.func_arg(argv[optind] + strlen(opt->name)
					      + 1);
			continue;
		}

		/* We wanted an argument, but there was none */
		if (optind + 1 >= argc)
			usage(stderr, EXIT_FAILURE);

		opt->handler.func_arg(argv[++optind]);
	}

	if (optind < argc) {
		/* Can't specify both a jar and a class file */
		if (jar_file)
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

	init_system_properties();
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
	if (init_threading()) {
		fprintf(stderr, "could not initialize threading\n");
		goto out_check_exception;
	}

	struct vm_class *vmc = classloader_load(classname);
	if (!vmc) {
		fprintf(stderr, "error: %s: could not load\n", classname);
		goto out;
	}

	if (vm_class_ensure_init(vmc)) {
		fprintf(stderr, "error: %s: couldn't initialize\n", classname);
		goto out_check_exception;
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

	void (*main_method_trampoline)(void)
		= vm_method_trampoline_ptr(vmm);
	main_method_trampoline();

out_check_exception:
	if (exception_occurred()) {
		vm_print_exception(exception_occurred());
		goto out;
	}
	status = EXIT_SUCCESS;

	vm_thread_wait_for_non_daemons();
out:
	return status;
}
