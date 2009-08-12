#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <zip.h>

#include "cafebabe/class.h"
#include "cafebabe/stream.h"

#include "vm/classloader.h"
#include "vm/preload.h"
#include "vm/class.h"
#include "vm/die.h"
#include "vm/backtrace.h"
#include "vm/trace.h"

#include "lib/string.h"
#include "lib/hash-map.h"

bool opt_trace_classloader;
static __thread int trace_classloader_level = 0;

static pthread_mutex_t classloader_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t classloader_cond = PTHREAD_COND_INITIALIZER;

static inline void trace_push(const char *class_name)
{
	assert(trace_classloader_level >= 0);

	if (opt_trace_classloader) {
		trace_printf("classloader: %*s%s\n",
			     trace_classloader_level, "", class_name);
		trace_flush();
	}

	++trace_classloader_level;
}

static inline void trace_pop(void)
{
	assert(trace_classloader_level >= 1);

	--trace_classloader_level;
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

char *get_classpath(void)
{
	struct classpath *cp;
	struct string *str;
	char *value;
	bool first;

	str = alloc_str();
	first = true;

	list_for_each_entry(cp, &classpaths, node) {
		if (!first) {
			str_append(str, ":");
			first = false;
		}

		str_append(str, "%s", cp->path);
	}

	value = strdup(str->value);
	free_str(str);

	return value;
}

static int add_dir_to_classpath(const char *dir)
{
	struct classpath *cp = malloc(sizeof *cp);
	if (!cp)
		return -ENOMEM;

	cp->type = CLASSPATH_DIR;
	cp->path = strdup(dir);
	if (!cp->path) {
		NOT_IMPLEMENTED;
		return -ENOMEM;
	}

	list_add_tail(&cp->node, &classpaths);
	return 0;
}

static int add_zip_to_classpath(const char *zip)
{
	int zip_error;

	struct classpath *cp = malloc(sizeof *cp);
	if (!cp)
		return -ENOMEM;

	cp->type = CLASSPATH_ZIP;
	cp->path = strdup(zip);
	if (!cp->path) {
		NOT_IMPLEMENTED;
		return -ENOMEM;
	}

	cp->zip = zip_open(zip, 0, &zip_error);
	if (!cp->zip) {
		NOT_IMPLEMENTED;
		return -1;
	}

	list_add_tail(&cp->node, &classpaths);
	return 0;
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
		if (!classpath_element) {
			NOT_IMPLEMENTED;
			break;
		}

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

struct classloader_class {
	bool loaded;
	struct vm_class *class;
};

static struct hash_map *classes;

void classloader_init(void)
{
	classes = alloc_hash_map(10000, string_hash, string_compare);
	if (!classes)
		error("failed to initialize class loader");
}

static struct classloader_class *lookup_class(const char *class_name)
{
	void *class;

	if (hash_map_get(classes, class_name, &class))
		return NULL;

	return class;
}

static char *dots_to_slash(const char *name)
{
	char *result = strdup(name);

	for (unsigned int i = 0, n = strlen(name); i < n; ++i) {
		if (result[i] == '.')
			result[i] = '/';
	}

	return result;
}

static char *class_name_to_file_name(const char *class_name)
{
	char *filename;

	if (asprintf(&filename, "%s.class", class_name) == -1) {
		NOT_IMPLEMENTED;
		return NULL;
	}

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
	if (cafebabe_class_init(class, &stream)) {
		NOT_IMPLEMENTED;
		goto out_stream;
	}

	cafebabe_stream_close(&stream);

	result = malloc(sizeof *result);
	if (result) {
		if (vm_class_link(result, class)) {
			NOT_IMPLEMENTED;
			return NULL;
		}
	}

	return result;

out_stream:
	cafebabe_stream_close(&stream);
out:
	return NULL;
}

static struct vm_class *load_class_from_dir(const char *dir, const char *file)
{
	struct vm_class *vmc;
	char *full_filename;

	if (asprintf(&full_filename, "%s/%s", dir, file) == -1) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	vmc = load_class_from_file(full_filename);
	free(full_filename);
	return vmc;
}

static struct vm_class *load_class_from_zip(struct zip *zip, const char *file)
{
	int zip_file_index;
	struct zip_stat zip_stat;
	struct zip_file *zip_file;
	uint8_t *zip_file_buf;

	struct cafebabe_stream stream;
	struct cafebabe_class *class;
	struct vm_class *result = NULL;

	zip_file_index = zip_name_locate(zip, file, 0);
	if (zip_file_index == -1)
		return NULL;

	if (zip_stat_index(zip, zip_file_index, 0, &zip_stat) == -1) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	zip_file_buf = malloc(zip_stat.size);
	if (!zip_file_buf) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	zip_file = zip_fopen_index(zip, zip_file_index, 0);
	if (!zip_file) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	/* Read the zipped class file */
	for (int offset = 0; offset != zip_stat.size;) {
		int ret;

		ret = zip_fread(zip_file,
			zip_file_buf + offset, zip_stat.size - offset);
		if (ret == -1) {
			NOT_IMPLEMENTED;
			return NULL;
		}

		offset += ret;
	}

	/* If this returns error, what can we do? We've got all the data we
	 * wanted, so there should be no point in returning the error. */
	zip_fclose(zip_file);

	cafebabe_stream_open_buffer(&stream, zip_file_buf, zip_stat.size);

	class = malloc(sizeof *class);
	if (cafebabe_class_init(class, &stream)) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	cafebabe_stream_close_buffer(&stream);

	result = malloc(sizeof *result);
	if (result) {
		if (vm_class_link(result, class)) {
			NOT_IMPLEMENTED;
			return NULL;
		}
	}

	return result;
}

static struct vm_class *load_class_from_classpath_file(const struct classpath *cp,
	const char *file)
{
	switch (cp->type) {
	case CLASSPATH_DIR:
		return load_class_from_dir(cp->path, file);
	case CLASSPATH_ZIP:
		return load_class_from_zip(cp->zip, file);
	}

	/* Should never reach this. */
	return NULL;
}

static struct vm_class *primitive_class_cache[VM_TYPE_MAX];

struct vm_class *classloader_load_primitive(const char *class_name)
{
	struct vm_class *class;
	int cache_index;

	cache_index = str_to_type(class_name);

	if (cache_index == J_VOID || cache_index == J_REFERENCE)
		return NULL;

	if (primitive_class_cache[cache_index])
		return primitive_class_cache[cache_index];

	class = malloc(sizeof *class);
	if (!class) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	class->primitive_vm_type = str_to_type(class_name);

	if (vm_class_link_primitive_class(class, class_name)) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	primitive_class_cache[cache_index] = class;

	return class;
}

static struct vm_class *load_array_class(const char *class_name)
{
	struct vm_class *array_class;
	char *elem_class_name;

	assert(class_name[0] == '[');

	array_class = malloc(sizeof *array_class);
	if (!array_class) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	elem_class_name =
		vm_class_get_array_element_class_name(class_name);
	if (!elem_class_name) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	if (vm_class_link_array_class(array_class, class_name)) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	if (str_to_type(class_name + 1) != J_REFERENCE) {
		array_class->array_element_class =
			classloader_load_primitive(elem_class_name);
	} else {
		array_class->array_element_class =
			classloader_load(elem_class_name);
	}

	free(elem_class_name);

	return array_class;
}

static struct vm_class *load_class(const char *class_name)
{
	struct vm_class *result;
	char *filename;

	if (class_name[0] == '[')
		return load_array_class(class_name);

	filename = class_name_to_file_name(class_name);
	if (!filename) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	result = load_class_from_file(filename);
	if (result)
		goto out_filename;

	struct classpath *cp;
	list_for_each_entry(cp, &classpaths, node) {
		result = load_class_from_classpath_file(cp, filename);
		if (result)
			break;
	}

out_filename:
	free(filename);

	return result;
}

/* XXX: Should use hash table or tree, not linear search. -Vegard */
struct vm_class *classloader_load(const char *class_name)
{
	struct vm_class *vmc;
	struct classloader_class *class;

	trace_push(class_name);

	char *slash_class_name = dots_to_slash(class_name);
	if (!slash_class_name) {
		trace_pop();
		return NULL;
	}

	pthread_mutex_lock(&classloader_mutex);

	class = lookup_class(slash_class_name);
	if (class) {
		/* If class is being loaded by another thread then wait
		 * until loading is completed. */
		while (!class->loaded)
			pthread_cond_wait(&classloader_cond, &classloader_mutex);

		vmc = class->class;
		goto out_unlock;
	}

	class = malloc(sizeof(*class));
	class->loaded = false;

	if (hash_map_put(classes, strdup(slash_class_name), class)) {
		NOT_IMPLEMENTED;
		vmc = NULL;
		goto out_unlock;
	}

	pthread_mutex_unlock(&classloader_mutex);

	/*
	 * XXX: We cannot hold classloader_mutex lock when calling
	 * load_class() because for example vm_class_init() might call
	 * classloader_load() for superclasses.
	 */
	vmc = load_class(slash_class_name);
	if (!vmc) {
		/*
		 * We should remove the entry from map but other threads
		 * might be waiting on it.
		 */
		NOT_IMPLEMENTED;
		vmc = NULL;
		goto out;
	}

	pthread_mutex_lock(&classloader_mutex);

	vmc->classloader = NULL;

	class->class = vmc;
	class->loaded = true;

	/* Tell other threads that the class has been loaded. Would it
	 * be worth to use a per-class condition variable for that? */
	pthread_cond_broadcast(&classloader_cond);

 out_unlock:
	pthread_mutex_unlock(&classloader_mutex);

 out:
	free(slash_class_name);
	trace_pop();
	return vmc;
}

/**
 * Returns class for a given name if it has already been loaded or
 * NULL otherwise. It may block if class it being loaded.
 */
struct vm_class *classloader_find_class(const char *name)
{
	struct vm_class *vmc;
	char *slash_class_name;
	struct classloader_class *class;

	slash_class_name = dots_to_slash(name);
	if (!slash_class_name) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	vmc = NULL;

	pthread_mutex_lock(&classloader_mutex);

	class = lookup_class(slash_class_name);
	if (class) {
		/* If class is being loaded by another thread then wait
		 * until loading is completed. */
		while (!class->loaded)
			pthread_cond_wait(&classloader_cond, &classloader_mutex);

		vmc = class->class;
	}

	free(slash_class_name);

	pthread_mutex_unlock(&classloader_mutex);
	return vmc;
}
