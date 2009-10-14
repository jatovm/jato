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

#include <valgrind/valgrind.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>
#include <regex.h>
#include <stdio.h>
#include <time.h>

#include "cafebabe/access.h"
#include "cafebabe/attribute_info.h"
#include "cafebabe/class.h"
#include "cafebabe/constant_pool.h"
#include "cafebabe/error.h"
#include "cafebabe/field_info.h"
#include "cafebabe/method_info.h"
#include "cafebabe/stream.h"

#include "runtime/stack-walker.h"
#include "runtime/runtime.h"
#include "runtime/unsafe.h"
#include "runtime/class.h"
#include "runtime/classloader.h"

#include "jit/compiler.h"
#include "jit/cu-mapping.h"
#include "jit/exception.h"
#include "jit/perf-map.h"
#include "jit/text.h"

#include "lib/list.h"

#include "vm/fault-inject.h"
#include "vm/classloader.h"
#include "vm/stack-trace.h"
#include "vm/reflection.h"
#include "vm/natives.h"
#include "vm/preload.h"
#include "vm/version.h"
#include "vm/itable.h"
#include "vm/method.h"
#include "vm/object.h"
#include "vm/signal.h"
#include "vm/static.h"
#include "vm/string.h"
#include "vm/system.h"
#include "vm/thread.h"
#include "vm/class.h"
#include "vm/call.h"
#include "vm/jar.h"
#include "vm/jni.h"
#include "vm/gc.h"
#include "vm/vm.h"

#include "arch/init.h"

static bool perf_enabled;
static char *exe_name;

/* Arguments passed to the main class.  */
static unsigned int nr_java_args;
static char **java_args;

/*
 * Enable JIT workarounds for valgrind.
 */
bool running_on_valgrind;

static void __attribute__((noreturn)) vm_exit(int status)
{
	clear_exception();
	vm_call_method(vm_java_lang_System_exit, status);
	if (exception_occurred())
		vm_print_exception(exception_occurred());

	error("System.exit() returned");
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

static struct system_properties_entry *find_system_property(const char *key)
{
	struct system_properties_entry *this;

	list_for_each_entry(this, &system_properties_list, list_node) {
		if (strcmp(this->key, key) == 0)
			return this;
	}

	return NULL;
}

static void add_system_property(char *key, char *value)
{
	struct system_properties_entry *ent;

	assert(key && value);

	ent = find_system_property(key);
	if (ent) {
		free(ent->value);
		free(key);

		ent->value = value;
		return;
	}

	ent = malloc(sizeof *ent);
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

	if (value == NULL)
		value = "";

	key_d = strdup(key);
	value_d = strdup(value);

	if (!key_d || !value_d)
		error("out of memory");

	add_system_property(key_d, value_d);
}

static void system_property_append_path(const char *key, const char *path)
{
	struct system_properties_entry *ent;

	ent = find_system_property(key);
	if (!ent) {
		add_system_property_const(key, path);
		return;
	}

	if (asprintf(&ent->value, "%s:%s", ent->value, path) < 0)
		error("out of memory");
}

struct system_property {
	const char *key;
	const char *value;
};

static struct system_property system_properties[] = {
	{ "java.vm.name",			"jato"				},
	{ "java.vm.vendor",			"Pekka Enberg"			},
	{ "java.vm.vendor.url",			"http://jatovm.sourceforge.net/"},
	{ "java.io.tmpdir",			"/tmp"				},
	{ "file.separator",			"/"				},
	{ "path.separator",			":"				},
	{ "line.separator",			"\n"				},
	{ "java.version",			"1.6"				},
	{ "java.vendor",			"GNU Classpath"			},
	{ "java.vendor.url",			"http://www.classpath.org"	},
	{ "java.vm.specification.version",	"1.0"				},
	{ "java.vm.specification.vendor",	"Sun Microsystems, Inc."	},
	{ "java.vm.specification.name",		"Java Virtual Machine Specification"},
	{ "java.specification.version",		"1.6"				},
	{ "java.specification.vendor",		"Sun Microsystems, Inc."	},
	{ "java.specification.name",		"Java Platform API Specification"},
	{ "java.class.version",			"48.0"				},
	{ "java.compiler",			"jato"				},
	{ "java.ext.dirs",			""				},
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

	add_system_property_const("java.library.path",
				  getenv("LD_LIBRARY_PATH"));

	char *cwd = get_current_dir_name();
	add_system_property_const("user.dir", cwd);
	free(cwd);

	add_system_property_const("user.name", getenv("USER"));
	add_system_property_const("user.home", getenv("HOME"));

	struct utsname info;

	uname(&info);
	add_system_property_const("os.arch", ARCH_NAME);
	add_system_property_const("os.name", info.sysname);
	add_system_property_const("os.version", info.release);

	add_system_property_const("gnu.cpu.endian", "little");
}


/**
 * This should be called after parsing of call arguments.
 */
static void init_configurable_system_properties(void)
{
	/* XXX: currently user defined -Djava.class.path=value is overriden. */
	char *cp = get_classpath();
	add_system_property_const("java.class.path", cp);
	add_system_property_const("java.boot.class.path", cp);
	free(cp);
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
		return;
	}

	if (!vm_class_is_array_class(src->class) ||
	    !vm_class_is_array_class(dest->class)) {
		signal_new_exception(vm_java_lang_ArrayStoreException, NULL);
		return;
	}

	src_elem_class = vm_class_get_array_element_class(src->class);
	dest_elem_class = vm_class_get_array_element_class(dest->class);
	if (!src_elem_class || !dest_elem_class) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return;
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
		return;
	}

	elem_size = get_vmtype_size(elem_type);
	memmove(dest->fields + dest_start * elem_size,
		src->fields + src_start * elem_size,
		len * elem_size);

	return;
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
		return NULL;
	}

	return vm_object_clone(object);
}

static struct vm_object *
native_vmobject_getclass(struct vm_object *object)
{
	if (!object) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return NULL;
	}

	assert(object->class);

	return object->class->object;
}

static int
native_vmfile_is_directory(struct vm_object *dirpath)
{
	char *dirpath_str;
	struct stat buf;

	if (!dirpath) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
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

static jint native_vmthread_interrupted(void)
{
	return vm_thread_interrupted(vm_thread_self());
}

static jint native_vmthread_isinterrupted(struct vm_object *vmthread)
{
	struct vm_thread *thread;

	thread = (struct vm_thread *)field_get_object(vmthread, vm_java_lang_VMThread_vmdata);
	return vm_thread_is_interrupted(thread);
}

static void native_vmthread_interrupt(struct vm_object *vmthread)
{
	struct vm_thread *thread;

	thread = (struct vm_thread *)field_get_object(vmthread, vm_java_lang_VMThread_vmdata);
	vm_thread_interrupt(thread);
}

static void native_vmobject_notify(struct vm_object *obj)
{
	vm_monitor_notify(&obj->monitor);

	if (exception_occurred())
		return;
}

static void native_vmobject_notify_all(struct vm_object *obj)
{
	vm_monitor_notify_all(&obj->monitor);

	if (exception_occurred())
		return;
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

static void native_vmobject_wait(struct vm_object *object, jlong ms, jint ns)
{
	if (ms == 0 && ns == 0)
		vm_monitor_wait(&object->monitor);
	else
		vm_monitor_timed_wait(&object->monitor, ms, ns);
}

static struct vm_object *native_vmstring_intern(struct vm_object *str)
{
	return vm_string_intern(str);
}

static jint native_atomiclong_vm_supports_cs8(void)
{
	return false;
}

static struct vm_native natives[] = {
	DEFINE_NATIVE("gnu/classpath/VMStackWalker", "getClassContext", native_vmstackwalker_getclasscontext),
	DEFINE_NATIVE("gnu/classpath/VMStackWalker", "getClassLoader", native_vmclass_getclassloader),
	DEFINE_NATIVE("gnu/classpath/VMSystemProperties", "preInit", native_vmsystemproperties_preinit),
	DEFINE_NATIVE("jato/internal/VM", "disableFault", native_vm_disable_fault),
	DEFINE_NATIVE("jato/internal/VM", "enableFault", native_vm_enable_fault),
	DEFINE_NATIVE("jato/internal/VM", "exit", native_vmruntime_exit),
	DEFINE_NATIVE("jato/internal/VM", "println", native_vmruntime_println),
	DEFINE_NATIVE("jato/internal/VM", "throwNullPointerException", native_vm_throw_null_pointer_exception),
	DEFINE_NATIVE("java/io/VMFile", "isDirectory", native_vmfile_is_directory),
	DEFINE_NATIVE("java/lang/VMClass", "forName", native_vmclass_forname),
	DEFINE_NATIVE("java/lang/VMClass", "getClassLoader", native_vmclass_getclassloader),
	DEFINE_NATIVE("java/lang/VMClass", "getComponentType", native_vmclass_getcomponenttype),
	DEFINE_NATIVE("java/lang/VMClass", "getDeclaredConstructors", native_vmclass_get_declared_constructors),
	DEFINE_NATIVE("java/lang/VMClass", "getDeclaredFields", native_vmclass_get_declared_fields),
	DEFINE_NATIVE("java/lang/VMClass", "getDeclaredMethods", native_vmclass_get_declared_methods),
	DEFINE_NATIVE("java/lang/VMClass", "getInterfaces", native_vmclass_get_interfaces),
	DEFINE_NATIVE("java/lang/VMClass", "getModifiers", native_vmclass_getmodifiers),
	DEFINE_NATIVE("java/lang/VMClass", "getName", native_vmclass_getname),
	DEFINE_NATIVE("java/lang/VMClass", "getSuperclass", native_vmclass_get_superclass),
	DEFINE_NATIVE("java/lang/VMClass", "isAnonymousClass", native_vmclass_is_anonymous_class),
	DEFINE_NATIVE("java/lang/VMClass", "isArray", native_vmclass_isarray),
	DEFINE_NATIVE("java/lang/VMClass", "isAssignableFrom", native_vmclass_is_assignable_from),
	DEFINE_NATIVE("java/lang/VMClass", "isInstance", native_vmclass_isinstance),
	DEFINE_NATIVE("java/lang/VMClass", "isInterface", native_vmclass_isinterface),
	DEFINE_NATIVE("java/lang/VMClass", "isPrimitive", native_vmclass_isprimitive),
	DEFINE_NATIVE("java/lang/VMClassLoader", "defineClass", native_vmclassloader_defineclass),
	DEFINE_NATIVE("java/lang/VMClassLoader", "findLoadedClass", native_vmclassloader_findloadedclass),
	DEFINE_NATIVE("java/lang/VMClassLoader", "getPrimitiveClass", native_vmclassloader_getprimitiveclass),
	DEFINE_NATIVE("java/lang/VMClassLoader", "loadClass", native_vmclassloader_loadclass),
	DEFINE_NATIVE("java/lang/VMObject", "clone", native_vmobject_clone),
	DEFINE_NATIVE("java/lang/VMObject", "getClass", native_vmobject_getclass),
	DEFINE_NATIVE("java/lang/VMObject", "notify", native_vmobject_notify),
	DEFINE_NATIVE("java/lang/VMObject", "notifyAll", native_vmobject_notify_all),
	DEFINE_NATIVE("java/lang/VMObject", "wait", native_vmobject_wait),
	DEFINE_NATIVE("java/lang/VMRuntime", "exit", native_vmruntime_exit),
	DEFINE_NATIVE("java/lang/VMRuntime", "mapLibraryName", native_vmruntime_maplibraryname),
	DEFINE_NATIVE("java/lang/VMRuntime", "nativeLoad", native_vmruntime_native_load),
	DEFINE_NATIVE("java/lang/VMRuntime", "runFinalizationForExit", native_vmruntime_run_finalization_for_exit),
	DEFINE_NATIVE("java/lang/VMString", "intern", native_vmstring_intern),
	DEFINE_NATIVE("java/lang/VMSystem", "arraycopy", native_vmsystem_arraycopy),
	DEFINE_NATIVE("java/lang/VMSystem", "identityHashCode", native_vmsystem_identityhashcode),
	DEFINE_NATIVE("java/lang/VMSystem", "nanoTime", native_vmsystem_nano_time),
	DEFINE_NATIVE("java/lang/VMThread", "currentThread", native_vmthread_current_thread),
	DEFINE_NATIVE("java/lang/VMThread", "interrupt", native_vmthread_interrupt),
	DEFINE_NATIVE("java/lang/VMThread", "interrupted", native_vmthread_interrupted),
	DEFINE_NATIVE("java/lang/VMThread", "isInterrupted", native_vmthread_isinterrupted),
	DEFINE_NATIVE("java/lang/VMThread", "start", native_vmthread_start),
	DEFINE_NATIVE("java/lang/VMThrowable", "fillInStackTrace", native_vmthrowable_fill_in_stack_trace),
	DEFINE_NATIVE("java/lang/VMThrowable", "getStackTrace", native_vmthrowable_get_stack_trace),
	DEFINE_NATIVE("java/lang/reflect/Constructor", "constructNative", native_constructor_construct_native),
	DEFINE_NATIVE("java/lang/reflect/Constructor", "getModifiersInternal", native_constructor_get_modifiers_internal),
	DEFINE_NATIVE("java/lang/reflect/Constructor", "getParameterTypes", native_constructor_get_parameter_types),
	DEFINE_NATIVE("java/lang/reflect/Field", "get", native_field_get),
	DEFINE_NATIVE("java/lang/reflect/Field", "getLong", native_field_get_long),
	DEFINE_NATIVE("java/lang/reflect/Field", "getModifiersInternal", native_field_get_modifiers_internal),
	DEFINE_NATIVE("java/lang/reflect/Field", "getType", native_field_gettype),
	DEFINE_NATIVE("java/lang/reflect/Field", "set", native_field_set),
	DEFINE_NATIVE("java/lang/reflect/Method", "getModifiersInternal", native_method_get_modifiers_internal),
	DEFINE_NATIVE("java/lang/reflect/Method", "getParameterTypes", native_method_get_parameter_types),
	DEFINE_NATIVE("java/lang/reflect/Method", "getReturnType", native_method_getreturntype),
	DEFINE_NATIVE("java/lang/reflect/Method", "invokeNative", native_method_invokenative),
	DEFINE_NATIVE("java/util/concurrent/atomic/AtomicLong", "VMSupportsCS8", native_atomiclong_vm_supports_cs8),
	DEFINE_NATIVE("sun/misc/Unsafe", "compareAndSwapInt", native_unsafe_compare_and_swap_int),
	DEFINE_NATIVE("sun/misc/Unsafe", "compareAndSwapLong", native_unsafe_compare_and_swap_long),
	DEFINE_NATIVE("sun/misc/Unsafe", "compareAndSwapObject", native_unsafe_compare_and_swap_object),
	DEFINE_NATIVE("sun/misc/Unsafe", "objectFieldOffset", native_unsafe_object_field_offset),
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

static void handle_version(void)
{
	fprintf(stdout, "Jato VM version %s\n", JATO_VERSION);
	exit(EXIT_SUCCESS);
}

static void handle_help(void)
{
	usage(stdout, EXIT_SUCCESS);
}

static void handle_classpath(const char *arg)
{
	classloader_add_to_classpath(arg);
}

enum operation {
	OPERATION_MAIN_CLASS,
	OPERATION_JAR_FILE,
};

static enum operation operation = OPERATION_MAIN_CLASS;

static char *classname;
static struct vm_jar *jar_file;

static void handle_jar(const char *arg)
{
	operation = OPERATION_JAR_FILE;

	/* Can't specify more than one jar file */
	if (jar_file)
		usage(stderr, EXIT_FAILURE);

	jar_file = vm_jar_open(arg);
	if (!jar_file) {
		fprintf(stdout, "Unable to open JAR file %s\n", arg);
		exit(EXIT_FAILURE);
	}

	const char *main_class = vm_jar_get_main_class(jar_file);
	if (!main_class) {
		fprintf(stdout, "Unable to look up main class in JAR file %s\n", arg);
		exit(EXIT_FAILURE);
	}

	classname = strdup(main_class);
	if (!classname)
		die("out of memory");

	/* XXX: Cheap solution. This can give funny results depending on where
	 * you put the -jar relative to the -classpath(s). Besides, we should
	 * save some memory and only open the zip file once. */
	classloader_add_to_classpath(arg);
}

static void handle_gc(void)
{
	gc_enabled = true;
}

static void handle_perf(void)
{
	perf_enabled = true;
}

/* @arg must be in the format package/name/Class.method(Lsignature;)V */
static void handle_trace_method(const char *arg)
{
	opt_trace_method = true;

	int err = regcomp(&method_trace_regex, arg, REG_EXTENDED | REG_NOSUB);
	if (err) {
		unsigned int size = regerror(err, &method_trace_regex, NULL, 0);
		char *errbuf = malloc(size);
		regerror(err, &method_trace_regex, errbuf, size);

		fprintf(stderr, "error: regcomp: %s\n", errbuf);
		free(errbuf);

		exit(EXIT_FAILURE);
	}
}

static void handle_trace_asm(void)
{
	opt_trace_machine_code = true;
	opt_trace_compile = true;
}

static void handle_trace_bytecode_offset(void)
{
	opt_trace_bytecode_offset = true;
}

static void handle_trace_classloader(void)
{
	opt_trace_classloader = true;
}

static void handle_trace_compile(void)
{
	opt_trace_compile = true;
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
	opt_trace_cfg = true;
	opt_trace_tree_ir = true;
	opt_trace_lir = true;
	opt_trace_liveness = true;
	opt_trace_regalloc = true;
	opt_trace_machine_code = true;
	opt_trace_magic_trampoline = true;
	opt_trace_bytecode_offset = true;
	opt_trace_compile = true;
}

static void handle_trace_bytecode(void)
{
	opt_trace_bytecode = true;
	opt_trace_compile = true;
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

	key = strdup(key);

	if (value)
		value = strdup(value);
	else
		value = strdup("");

	if (!key || !value)
		error("out of memory");

	add_system_property(key, value);
 out:
	free(str);
}

static void handle_verbose_gc(void)
{
	verbose_gc = true;
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
	DEFINE_OPTION("version",	handle_version),
	DEFINE_OPTION("h",		handle_help),
	DEFINE_OPTION("help",		handle_help),

	DEFINE_OPTION_ADJACENT_ARG("D",	handle_define),

	DEFINE_OPTION_ARG("classpath",	handle_classpath),
	DEFINE_OPTION_ARG("cp",		handle_classpath),
	DEFINE_OPTION_ARG("jar",	handle_jar),
	DEFINE_OPTION("verbose:gc",	handle_verbose_gc),

	DEFINE_OPTION("Xgc",			handle_gc),
	DEFINE_OPTION("Xperf",			handle_perf),

	DEFINE_OPTION_ARG("Xtrace:method",	handle_trace_method),

	DEFINE_OPTION("Xtrace:asm",		handle_trace_asm),
	DEFINE_OPTION("Xtrace:bytecode",	handle_trace_bytecode),
	DEFINE_OPTION("Xtrace:bytecode-offset",	handle_trace_bytecode_offset),
	DEFINE_OPTION("Xtrace:classloader",	handle_trace_classloader),
	DEFINE_OPTION("Xtrace:compile",		handle_trace_compile),
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

	if (operation == OPERATION_MAIN_CLASS) {
		if (optind >= argc)
			usage(stderr, EXIT_FAILURE);

		classname = argv[optind++];
	}

	if (optind < argc) {
		nr_java_args	= argc - optind;
		java_args	= &argv[optind];
	}
}

static struct vm_object *get_system_class_loader(void)
{
	if (vm_class_ensure_init(vm_java_lang_ClassLoader))
		return NULL;

	return vm_call_method_object(vm_java_lang_ClassLoader_getSystemClassLoader);
}

static int
do_main_class(void)
{
	struct vm_object *loader;

	loader = get_system_class_loader();
	if (!loader)
		return -1;

	struct vm_class *vmc = classloader_load(loader, classname);
	if (!vmc) {
		fprintf(stderr, "error: %s: could not load\n", classname);
		return -1;
	}

	if (vm_class_ensure_init(vmc)) {
		fprintf(stderr, "error: %s: couldn't initialize\n", classname);
		return -1;
	}

	struct vm_method *vmm = vm_class_get_method_recursive(vmc,
		"main", "([Ljava/lang/String;)V");
	if (!vmm) {
		fprintf(stderr, "error: %s: no main method\n", classname);
		return -1;
	}

	if (!vm_method_is_static(vmm)) {
		fprintf(stderr, "error: %s: main method not static\n",
			classname);
		return -1;
	}

	struct vm_object *args;

	args = vm_object_alloc_array(vm_array_of_java_lang_String, nr_java_args);
	if (!args)
		die("out of memory");

	for (unsigned int i = 0; i < nr_java_args; i++) {
		struct vm_object *arg;

		arg = vm_object_alloc_string_from_c(java_args[i]);

		array_set_field_object(args, i, arg);
	}

	void (*main_method_trampoline)(void *)
		= vm_method_trampoline_ptr(vmm);
	main_method_trampoline(args);

	return 0;
}

static int
do_jar_file(void)
{
	/* XXX: This stub should be expanded in the future; see comment in
	 * handle_jar(). */
	return do_main_class();
}

struct gnu_classpath_config {
	const char *glibj;
	const char *lib;
};

struct gnu_classpath_config gnu_classpath_configs[] = {
	{
		"/usr/local/classpath/share/classpath/glibj.zip",
		"/usr/local/classpath/lib/classpath"
	},
	{
		"/usr/share/classpath/glibj.zip",
		"/usr/lib/classpath/"
	},
};

static void gnu_classpath_autodiscovery(void)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(gnu_classpath_configs); i++) {
		struct gnu_classpath_config *config = &gnu_classpath_configs[i];

		if (try_to_add_zip_to_classpath(config->glibj) < 0)
			continue;

		system_property_append_path("java.library.path", config->lib);
		break;
	}
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

	if (RUNNING_ON_VALGRIND) {
		printf("JIT: Enabling workarounds for valgrind.\n");
		running_on_valgrind = true;
	}

	arch_init();
	init_literals_hash_map();
	init_system_properties();

	parse_options(argc, argv);

	classloader_init();

	init_vm_objects();
	init_vm_monitors();

	jit_text_init();

	if (perf_enabled)
		perf_map_open();

	setup_signal_handlers();
	init_cu_mapping();
	init_exceptions();

	jit_init_natives();

	gc_init();
	static_fixup_init();
	vm_jni_init();

	gnu_classpath_autodiscovery();

	/* Search $CLASSPATH last. */
	char *classpath = getenv("CLASSPATH");
	if (classpath)
		classloader_add_to_classpath(classpath);

	init_configurable_system_properties();

	if (preload_vm_classes()) {
		NOT_IMPLEMENTED;
		exit(EXIT_FAILURE);
	}

	init_stack_trace_printing();
	if (init_threading()) {
		fprintf(stderr, "could not initialize threading\n");
		goto out_check_exception;
	}

	switch (operation) {
	case OPERATION_MAIN_CLASS:
		do_main_class();
		break;
	case OPERATION_JAR_FILE:
		do_jar_file();
		break;
	}

out_check_exception:
	if (exception_occurred()) {
		vm_print_exception(exception_occurred());
		goto out;
	}
	status = EXIT_SUCCESS;

	vm_thread_wait_for_non_daemons();
out:
	vm_exit(status);

	/* XXX: We should not get there */
	return EXIT_FAILURE;
}
