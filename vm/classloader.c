#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

#include <zip.h>

#include <cafebabe/class.h>
#include <cafebabe/stream.h>

#include <vm/class.h>
#include <vm/classloader.h>
#include <vm/preload.h>

bool opt_trace_classloader;
static int trace_classloader_level = 0;

static inline void trace_push(const char *class_name)
{
	assert(trace_classloader_level >= 0);

	if (opt_trace_classloader) {
		fprintf(stderr, "classloader: %*s%s\n",
			trace_classloader_level, "", class_name);
	}

	++trace_classloader_level;
}

static inline void trace_pop()
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

	union {
		const char *dir;
		struct zip *zip;
	};
};

/* These are the directories we search for classes */
struct list_head classpaths = LIST_HEAD_INIT(classpaths);

static int add_dir_to_classpath(const char *dir)
{
	struct classpath *cp = malloc(sizeof *cp);
	if (!cp)
		return -ENOMEM;

	cp->type = CLASSPATH_DIR;
	cp->dir= strdup(dir);
	if (!cp->dir) {
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
	cp->zip = zip_open(zip, 0, &zip_error);
	if (!cp->zip) {
		NOT_IMPLEMENTED;
		return -1;
	}

	list_add_tail(&cp->node, &classpaths);
	return 0;
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
			/* XXX: We need to figure out what the semantics for
			 * this function should be on error. IIRC, regular
			 * java just ignores invalid classpath components.
			 * We probably _shouldn't_ ignore those that we
			 * couldn't add because we are out of memory. Also,
			 * do we return error even if _some_ paths were
			 * added successfully? */

			if (S_ISDIR(st.st_mode))
				add_dir_to_classpath(classpath_element);

			if (S_ISREG(st.st_mode))
				add_zip_to_classpath(classpath_element);
		}

		free(classpath_element);
	}

	return 0;
}

struct classloader_class {
	struct vm_class *class;
};

static struct classloader_class *classes;
static unsigned long max_classes;
static unsigned long nr_classes;

static struct classloader_class *lookup_class(const char *class_name)
{
	unsigned long i;

	for (i = 0; i < nr_classes; ++i) {
		struct classloader_class *class = &classes[i];

		if (!strcmp(class->class->name, class_name))
			return class;
	}

	return NULL;
}

char *class_name_to_file_name(const char *class_name)
{
	char *filename;

	if (asprintf(&filename, "%s.class", class_name) == -1) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	for (unsigned int i = 0, n = strlen(class_name); i < n; ++i) {
		if (filename[i] == '.')
			filename[i] = '/';
	}

	return filename;
}

struct vm_class *load_class_from_file(const char *filename)
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

struct vm_class *load_class_from_dir(const char *dir, const char *file)
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

struct vm_class *load_class_from_zip(struct zip *zip, const char *file)
{
	int zip_file_index;
	struct zip_stat zip_stat;
	struct zip_file *zip_file;
	uint8_t *zip_file_buf;

	struct cafebabe_stream stream;
	struct cafebabe_class *class;
	struct vm_class *result = NULL;

	zip_file_index = zip_name_locate(zip, file, 0);
	if (zip_file_index == -1) {
		NOT_IMPLEMENTED;
		return NULL;
	}

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

struct vm_class *load_class_from_classpath_file(const struct classpath *cp,
	const char *file)
{
	switch (cp->type) {
	case CLASSPATH_DIR:
		return load_class_from_dir(cp->dir, file);
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
	class->kind = VM_CLASS_KIND_PRIMITIVE;

	if (vm_class_link_bogus_class(class, class_name)) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	primitive_class_cache[cache_index] = class;

	return class;
}

struct vm_class *load_array_class(const char *class_name)
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

	array_class->kind = VM_CLASS_KIND_ARRAY;

	if (vm_class_link_bogus_class(array_class, class_name)) {
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

struct vm_class *load_class(const char *class_name)
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
	if (result)
		result->source_file_name = filename;
	else
		free(filename);

	return result;
}

/* XXX: Should use hash table or tree, not linear search. -Vegard */
struct vm_class *classloader_load(const char *class_name)
{
	struct classloader_class *class;
	struct vm_class *vmc;
	struct classloader_class *new_array;
	unsigned long new_max_classes;

	trace_push(class_name);

	class = lookup_class(class_name);
	if (class) {
		vmc = class->class;
		goto out;
	}

	vmc = load_class(class_name);
	if (!vmc) {
		NOT_IMPLEMENTED;
		vmc = NULL;
		goto out;
	}

	if (nr_classes == max_classes) {
		new_max_classes = 1 + max_classes * 2;
		new_array = realloc(classes,
			new_max_classes * sizeof(struct classloader_class));
		if (!new_array) {
			NOT_IMPLEMENTED;
			vmc = NULL;
			goto out;
		}

		max_classes = new_max_classes;
		classes = new_array;
	}

	class = &classes[nr_classes++];
	class->class = vmc;

out:
	trace_pop();
	return vmc;
}
