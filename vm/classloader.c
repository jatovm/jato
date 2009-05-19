#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>

#include <cafebabe/class.h>
#include <cafebabe/stream.h>

#include <vm/class.h>
#include <vm/classloader.h>

struct classloader_class {
	char *class_name;
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

		if (!strcmp(class->class_name, class_name))
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

	fprintf(stderr, "classloader: trying to load file '%s'...\n", filename);

	if (cafebabe_stream_open(&stream, filename)) {
		NOT_IMPLEMENTED;
		goto out;
	}

	class = malloc(sizeof *class);
	if (cafebabe_class_init(class, &stream)) {
		NOT_IMPLEMENTED;
		goto out_stream;
	}

	cafebabe_stream_close(&stream);

	result = malloc(sizeof *result);
	if (result)
		vm_class_init(result, class);

	return result;

out_stream:
	cafebabe_stream_close(&stream);
out:
	return NULL;
}

struct vm_class *load_class(const char *class_name)
{
	struct vm_class *result;
	char *filename;
	const char *classpath;

	fprintf(stderr, "classloader: trying to load class '%s'...\n", class_name);

	filename = class_name_to_file_name(class_name);
	if (!filename) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	result = load_class_from_file(filename);
	if (result) {
		/* XXX: Free filename */
		goto out_filename;
	}

	classpath = getenv("CLASSPATH");
	if (!classpath) {
		NOT_IMPLEMENTED;
		result = NULL;
		goto out_filename;
	}

	size_t i = 0;
	while (classpath[i]) {
		size_t n;
		char *full_filename;

		n = strspn(classpath + i, ":");
		i += n;

		n = strcspn(classpath + i, ":");
		if (n == 0)
			continue;

		if (asprintf(&full_filename, "%.*s/%s",
			n, classpath + i, filename) == -1)
		{
			NOT_IMPLEMENTED;
			result = NULL;
			goto out_filename;
		}

		i += n;

		result = load_class_from_file(full_filename);
		free(full_filename);

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
	struct classloader_class *class;
	struct vm_class *vmc;
	struct classloader_class *new_array;
	unsigned long new_max_classes;

	class = lookup_class(class_name);
	if (class)
		return class->class;

	vmc = load_class(class_name);
	if (!vmc) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	if (nr_classes == max_classes) {	
		new_max_classes = 1 + max_classes * 2;
		new_array = realloc(classes,
			new_max_classes * sizeof(struct classloader_class));
		if (!new_array) {
			NOT_IMPLEMENTED;
			return NULL;
		}

		max_classes = new_max_classes;
		classes = new_array;
	}

	class = &classes[nr_classes++];
	class->class_name = strdup(class_name);
	class->class = vmc;

	return vmc;
}
