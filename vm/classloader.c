#include "vm/classloader.h"

#include "cafebabe/stream.h"
#include "cafebabe/class.h"

#include "jit/exception.h"

#include "vm/reflection.h"
#include "vm/backtrace.h"
#include "vm/preload.h"
#include "vm/errors.h"
#include "vm/thread.h"
#include "vm/class.h"
#include "vm/trace.h"
#include "vm/utf8.h"
#include "vm/call.h"
#include "vm/die.h"

#include "lib/hash-map.h"
#include "lib/string.h"
#include "lib/zip.h"

#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <stdio.h>

bool opt_trace_classloader;

static pthread_mutex_t classloader_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t classloader_cond = PTHREAD_COND_INITIALIZER;

static inline void trace_push(struct vm_object *loader, const char *class_name)
{
	assert(vm_get_exec_env()->trace_classloader_level >= 0);

	if (opt_trace_classloader) {
		trace_printf("classloader: %p %*s%s\n", loader,
				vm_get_exec_env()->trace_classloader_level, "", class_name);
		trace_flush();
	}

	vm_get_exec_env()->trace_classloader_level += 1;
}

static inline void trace_pop(void)
{
	assert(vm_get_exec_env()->trace_classloader_level >= 1);
	vm_get_exec_env()->trace_classloader_level -= 1;
}

enum classpath_type {
	CLASSPATH_DIR,
	CLASSPATH_ZIP,
};

struct classpath {
	struct list_head node;

	enum classpath_type type;

	const char *path;
	struct zip *zip;
};

/* These are the directories we search for classes */
struct list_head classpaths = LIST_HEAD_INIT(classpaths);

void classloader_destroy(void)
{
	struct classpath *cp, *next;

	list_for_each_entry_safe(cp, next, &classpaths, node) {
		if (cp->type == CLASSPATH_ZIP)
			zip_close(cp->zip);

		list_del(&cp->node);
		free((void *) cp->path);
		free(cp);
	}
}

static int add_dir_to_classpath(const char *dir)
{
	struct classpath *cp = malloc(sizeof *cp);
	if (!cp)
		return -ENOMEM;

	cp->type = CLASSPATH_DIR;
	cp->path = strdup(dir);
	if (!cp->path) {
		free(cp);
		return -ENOMEM;
	}

	list_add_tail(&cp->node, &classpaths);
	return 0;
}

static int add_zip_to_classpath(const char *zip)
{
	int err;

	struct classpath *cp = malloc(sizeof *cp);
	if (!cp)
		return -ENOMEM;

	cp->type = CLASSPATH_ZIP;
	cp->path = strdup(zip);
	if (!cp->path) {
		err = -ENOMEM;
		goto error_free_cp;
	}

	cp->zip = zip_open(zip);
	if (!cp->zip) {
		err = -1;
		goto error_free_path;
	}

	list_add_tail(&cp->node, &classpaths);
	return 0;

error_free_path:
	free((void *) cp->path);
error_free_cp:
	free(cp);

	return err;
}

int try_to_add_zip_to_classpath(const char *zip)
{
	struct stat st;

	if (stat(zip, &st) != 0)
		return -EINVAL;

	if (!S_ISREG(st.st_mode))
		return -EINVAL;

	return add_zip_to_classpath(zip);
}

int classloader_add_to_classpath(const char *classpath)
{
	int i = 0;

	assert(classpath);

	while (classpath[i]) {
		size_t n;
		char *classpath_element;
		struct stat st;

		n = strspn(classpath + i, ":");
		i += n;

		n = strcspn(classpath + i, ":");
		if (n == 0)
			continue;

		classpath_element = strndup(classpath + i, n);
		if (!classpath_element)
			break;

		i += n;

		if (!stat(classpath_element, &st)) {
			if (S_ISDIR(st.st_mode))
				add_dir_to_classpath(classpath_element);
			else if (S_ISREG(st.st_mode))
				add_zip_to_classpath(classpath_element);
			else {
				warn("'%s' is not a regular file or a directory", classpath_element);
			}
		} else
			warn("'%s' does not exist", classpath_element);

		free(classpath_element);
	}

	return 0;
}

enum class_load_status {
	CLASS_LOADING,
	CLASS_LOADED,
	CLASS_NOT_FOUND,
};

struct classes_key {
	struct string		*class_name;
	struct vm_object	*classloader;
};

struct classloader_class {
	enum class_load_status status;
	struct vm_class *class;

	/* number of threads waiting for a class. */
	unsigned long nr_waiting;
	struct vm_thread *loading_thread;
	struct vm_object *classloader;

	struct classes_key key;
};

static struct hash_map *classes;

static unsigned long classes_key_hash(const void *key)
{
	const struct classes_key *classes_key = key;

	return pointer_key.hash(classes_key->class_name) + 31 * pointer_key.hash(classes_key->classloader);
}

static bool classes_key_equals(const void *key1, const void *key2)
{
	const struct classes_key *classes_key1 = key1;
	const struct classes_key *classes_key2 = key2;

	if (classes_key1->classloader != classes_key2->classloader)
		return false;

	return classes_key1->class_name == classes_key2->class_name;
}

static struct key_operations classes_key_ops = {
	.hash	= &classes_key_hash,
	.equals	= &classes_key_equals
};

void classloader_init(void)
{
	classes = alloc_hash_map(&classes_key_ops);
	if (!classes)
		error("failed to initialize class loader");
}

static struct classloader_class *
lookup_class(struct vm_object *loader, struct string *class_name)
{
	void *class;
	struct classes_key key;

	key.class_name  = class_name;
	key.classloader = loader;

	if (hash_map_get(classes, &key, &class))
		return NULL;

	return class;
}

static void remove_class(struct vm_object *loader, struct string *class_name)
{
	struct classes_key key;

	key.class_name  = class_name;
	key.classloader = loader;

	hash_map_remove(classes, &key);
}

static char *class_name_to_file_name(const char *class_name)
{
	char *filename;

	if (asprintf(&filename, "%s.class", class_name) == -1)
		return NULL;

	return filename;
}

static struct vm_class *load_class_from_file(const char *filename)
{
	struct cafebabe_stream stream;
	struct cafebabe_class *class;
	struct vm_class *result = NULL;

	if (cafebabe_stream_open(&stream, filename))
		goto out;

	class = malloc(sizeof *class);
	if (!class)
		goto error_close_stream;

	if (cafebabe_class_init(class, &stream))
		goto error_free_class;

	result = vm_zalloc(sizeof *result);
	if (!result)
		goto error_free_class;

	if (vm_class_link(result, class))
		goto error_free_class;

	cafebabe_stream_close(&stream);

	return result;

error_free_class:
	free(class);
error_close_stream:
	cafebabe_stream_close(&stream);
out:
	return NULL;
}

static struct vm_class *load_class_from_dir(const char *dir, const char *class_name)
{
	struct vm_class *vmc;
	char *full_filename;
	char *filename;

	filename = class_name_to_file_name(class_name);
	if (!filename)
		return NULL;

	if (asprintf(&full_filename, "%s/%s", dir, filename) == -1)
		return NULL;

	free(filename);

	vmc = load_class_from_file(full_filename);
	free(full_filename);
	return vmc;
}

static struct vm_class *load_class_from_zip(struct zip *zip, struct string *class_name)
{
	struct cafebabe_stream stream;
	struct cafebabe_class *class;
	struct vm_class *result = NULL;
	struct zip_entry *zip_entry;
	void *zip_file_buf;

	zip_entry = zip_entry_find_class(zip, class_name);
	if (!zip_entry)
		return NULL;

	zip_file_buf = zip_entry_data(zip, zip_entry);
	if (!zip_file_buf)
		return NULL;

	cafebabe_stream_open_buffer(&stream, zip_file_buf, zip_entry->uncomp_size);

	class = malloc(sizeof *class);
	if (cafebabe_class_init(class, &stream))
		goto error_free_buf;

	cafebabe_stream_close_buffer(&stream);

	result = vm_zalloc(sizeof *result);
	if (result) {
		if (vm_class_link(result, class))
			goto error_free_class;
	}

	free(zip_file_buf);
	return result;

error_free_class:
	free(class);
error_free_buf:
	free(zip_file_buf);

	return NULL;
}

static struct vm_class *
load_class_from_classpath_file(const struct classpath *cp, struct string *class_name)
{
	switch (cp->type) {
	case CLASSPATH_DIR:
		return load_class_from_dir(cp->path, class_name->value);
	case CLASSPATH_ZIP:
		return load_class_from_zip(cp->zip, class_name);
	}

	/* Should never reach this. */
	return NULL;
}

static struct vm_class *primitive_class_cache[VM_TYPE_MAX];

static enum vm_type vm_type_for_primitive_class_name(const char *name)
{
	if (!strcmp(name,"boolean"))
		return J_BOOLEAN;

	if (!strcmp(name,"byte"))
		return J_BYTE;

	if (!strcmp(name,"short"))
		return J_SHORT;

	if (!strcmp(name,"char"))
		return J_CHAR;

	if (!strcmp(name,"int"))
		return J_INT;

	if (!strcmp(name,"long"))
		return J_LONG;

	if (!strcmp(name,"float"))
		return J_FLOAT;

	if (!strcmp(name,"double"))
		return J_DOUBLE;

	if (!strcmp(name,"void"))
		return J_VOID;

	return VM_TYPE_MAX;
}

struct vm_class *classloader_load_primitive(const char *class_name)
{
	struct vm_class *class;
	enum vm_type type;

	type = vm_type_for_primitive_class_name(class_name);
	if (type == VM_TYPE_MAX)
		error("invalid primitive class name: %s", class_name);

	if (primitive_class_cache[type])
		return primitive_class_cache[type];

	class = vm_zalloc(sizeof *class);
	if (!class)
		return throw_oom_error();

	class->classloader = NULL;
	class->primitive_vm_type = type;

	if (vm_class_link_primitive_class(class, class_name))
		return NULL;

	primitive_class_cache[type] = class;

	return class;
}

static struct vm_class *
load_array_class(struct vm_object *loader, const char *class_name)
{
	struct vm_class *array_class;
	struct vm_class *elem_class;
	char *elem_class_name;

	assert(class_name[0] == '[');

	array_class = vm_zalloc(sizeof *array_class);
	if (!array_class)
		return NULL;

	elem_class_name =
		vm_class_get_array_element_class_name(class_name);
	if (!elem_class_name)
		return throw_oom_error();

	enum vm_type type = str_to_type(class_name + 1);

	if (type != J_REFERENCE)
		elem_class = primitive_class_cache[type];
	else
		elem_class = classloader_load(loader, elem_class_name);

	if (!elem_class) {
		vm_free(array_class);
		return NULL;
	}

	array_class->classloader = elem_class->classloader;

	if (vm_class_link_array_class(array_class, elem_class, class_name))
		return throw_oom_error();

	free(elem_class_name);
	return array_class;
}

static struct vm_class *
load_class_with(struct vm_object *loader, const char *name)
{
	struct vm_object *name_string;
	struct vm_object *clazz;
	struct vm_class *vmc;

	char *dots_name = slash_to_dots(name);

	name_string = vm_object_alloc_string_from_c(dots_name);
	free(dots_name);
	if (!name_string)
		return rethrow_exception();

	clazz = vm_call_method_this_object(vm_java_lang_ClassLoader_loadClass,
					   loader, name_string);
	if (!clazz || exception_occurred())
		return NULL;

	vmc = vm_object_to_vm_class(clazz);
	if (!vmc)
		return NULL;

	return vmc;
}

/*
 * Load non-array class with bootstrap classloader
 */
static struct vm_class *load_class(struct string *class_name)
{
	struct vm_class *result = NULL;
	struct classpath *cp;

	list_for_each_entry(cp, &classpaths, node) {
		result = load_class_from_classpath_file(cp, class_name);
		if (result)
			break;
	}

	if (result)
		result->classloader = NULL;

	return result;
}

static struct classloader_class *
find_class(struct vm_object *loader, struct string *class_name)
{
	struct classloader_class *class;

	class = lookup_class(loader, class_name);
	if (class) {
		/*
		 * If class is being loaded by current thread then we
		 * should return NULL. We do this because class
		 * loading might be delegated to external class
		 * loaders which might query the VM before loading.
		 */
		if (class->status == CLASS_LOADING &&
		    class->loading_thread == vm_thread_self())
			return NULL;

		/*
		 * If class is being loaded by another thread then
		 * wait until loading is completed.
		 */

		++class->nr_waiting;
		while (class->status == CLASS_LOADING)
			pthread_cond_wait(&classloader_cond, &classloader_mutex);
		--class->nr_waiting;

		if (class->status == CLASS_NOT_FOUND && !class->nr_waiting) {
			remove_class(loader, class_name);
			vm_free(class);
			class = NULL;
		}
	}

	return class;
}

static struct vm_class *
load_last_array_elem_class(struct vm_object *loader, const char *class_name)
{
	while (class_name[1] == '[')
		class_name++;

	char *elem_name =
		vm_class_get_array_element_class_name(class_name);

	struct vm_class *result;

	enum vm_type type = str_to_type(class_name + 1);

	if (type != J_REFERENCE)
		result = primitive_class_cache[type];
	else
		result = classloader_load(loader, elem_name);

	free(elem_name);
	return result;
}

/**
 * Loads a class of given name. if @loader is not NULL then
 * loading of a class will be delegated to @loader.
 *
 * If @class_name refers to an array class then @loader
 * will be called only for array element class.
 *
 * NOTE NOTE NOTE!!! The caller is expected to have called dots_to_slash() for
 * @class_name or ensuring that @class_name is in slash form in some other way.
 */
struct vm_class *
classloader_load(struct vm_object *loader, const char *klass_name)
{
	struct classloader_class *class;
	struct string *class_name;
	struct vm_class *vmc;

	trace_push(loader, klass_name);

	class_name = string_intern_cstr(klass_name);

	vmc = NULL;

	/*
	 * Array classes have classloader set to the classloader of
	 * its elements. Primitive types are always loaded with
	 * bootstrap classloader.
	 */
	if (loader && is_primitive_array(klass_name))
		loader = NULL;

	if (loader) {
		if (!is_array(klass_name)) {
			vmc = load_class_with(loader, klass_name);
			goto out;
		}

		/*
		 * Array classes are created by VM but the defining
		 * classloader for them is set to the class loader of
		 * array element. We must determine it before calling
		 * find_class() because we do not know yet what will
		 * be a defining classloader of array element class.
		 */
		struct vm_class *elem_class =
			load_last_array_elem_class(loader, klass_name);

		if (!elem_class)
			goto out;

		loader = elem_class->classloader;
	}

	pthread_mutex_lock(&classloader_mutex);

	class = find_class(loader, class_name);
	if (class) {
		if (class->status == CLASS_LOADED)
			vmc = class->class;

		goto out_unlock;
	}

	class = vm_zalloc(sizeof(*class));
	class->status = CLASS_LOADING;
	class->nr_waiting = 0;
	class->loading_thread = vm_thread_self();
	class->key.classloader = loader;
	class->key.class_name = class_name;

	if (hash_map_put(classes, &class->key, class)) {
		vmc = NULL;
		goto out_unlock;
	}

	/*
	 * XXX: We cannot hold classloader_mutex lock when calling
	 * load_class() because for example vm_class_init() might call
	 * classloader_load() for superclasses.
	 */
	pthread_mutex_unlock(&classloader_mutex);

	if (is_array(klass_name))
		vmc = load_array_class(loader, klass_name);
	else
		vmc = load_class(class_name);

	pthread_mutex_lock(&classloader_mutex);

	if (!vmc) {
		/*
		 * If there are other threads waiting for class to be
		 * loaded then do not remove the entry. Last thread
		 * removes the entry.
		 */
		if (class->nr_waiting == 0) {
			remove_class(loader, class_name);
			vm_free(class);
		} else {
			class->status = CLASS_NOT_FOUND;
		}
	} else {
		class->class = vmc;
		class->status = CLASS_LOADED;
	}

	pthread_cond_broadcast(&classloader_cond);

 out_unlock:
	pthread_mutex_unlock(&classloader_mutex);
 out:
	trace_pop();
	return vmc;
}

/**
 * Returns class for a given name if it has already been loaded or
 * NULL otherwise. It may block if class it being loaded.
 */
struct vm_class *
classloader_find_class(struct vm_object *loader, const char *name)
{
	struct vm_class *vmc;
	char *slash_class_name;
	struct classloader_class *class;
	struct string *class_name;

	slash_class_name = dots_to_slash(name);
	if (!slash_class_name)
		return NULL;

	vmc = NULL;

	class_name = string_intern_cstr(slash_class_name);

	pthread_mutex_lock(&classloader_mutex);

	class = find_class(loader, class_name);
	if (class && class->status == CLASS_LOADED)
		vmc = class->class;

	free(slash_class_name);

	pthread_mutex_unlock(&classloader_mutex);
	return vmc;
}

int classloader_add_to_cache(struct vm_object *loader, struct vm_class *vmc)
{
	struct classloader_class *class;

	class = vm_zalloc(sizeof(*class));
	if (!class)
		return -ENOMEM;

	class->class = vmc;
	class->status = CLASS_LOADED;
	class->nr_waiting = 0;
	class->loading_thread = vm_thread_self();
	class->key.classloader = loader;
	class->key.class_name = string_intern_cstr(vmc->name);

	pthread_mutex_lock(&classloader_mutex);

	if (hash_map_put(classes, &class->key, class)) {
		pthread_mutex_unlock(&classloader_mutex);
		vm_free(class);
		return -ENOMEM;
	}

	pthread_mutex_unlock(&classloader_mutex);
	return 0;
}

struct vm_object *get_system_class_loader(void)
{
	if (vm_class_ensure_init(vm_java_lang_ClassLoader))
		return NULL;

	return vm_call_method_object(vm_java_lang_ClassLoader_getSystemClassLoader);
}
