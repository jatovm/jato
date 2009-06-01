#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>

#include <zip.h>

#include <cafebabe/class.h>
#include <cafebabe/stream.h>

#include <vm/class.h>
#include <vm/classloader.h>

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
		if (vm_class_init(result, class)) {
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

struct vm_class *load_class_from_zip(const char *zipfile, const char *file)
{
	int zip_error;
	struct zip *zip;
	int zip_file_index;
	struct zip_stat zip_stat;
	struct zip_file *zip_file;
	uint8_t *zip_file_buf;

	struct cafebabe_stream stream;
	struct cafebabe_class *class;
	struct vm_class *result = NULL;

	zip = zip_open(zipfile, 0, &zip_error);
	if (!zip) {
		NOT_IMPLEMENTED;
		return NULL;
	}

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
	zip_close(zip);

	cafebabe_stream_open_buffer(&stream, zip_file_buf, zip_stat.size);

	class = malloc(sizeof *class);
	if (cafebabe_class_init(class, &stream)) {
		NOT_IMPLEMENTED;
		return NULL;
	}

	cafebabe_stream_close_buffer(&stream);

	result = malloc(sizeof *result);
	if (result) {
		if (vm_class_init(result, class)) {
			NOT_IMPLEMENTED;
			return NULL;
		}
	}

	return result;
}

struct vm_class *load_class_from_path_file(const char *path, const char *file)
{
	struct stat st;

	if (stat(path, &st) == -1) {
		/* Doesn't exist or not accessible */
		return NULL;
	}

	if (S_ISDIR(st.st_mode)) {
		/* Directory */
		struct vm_class *vmc;
		char *full_filename;

		if (asprintf(&full_filename, "%s/%s", path, file) == -1) {
			NOT_IMPLEMENTED;
			return NULL;
		}

		vmc = load_class_from_file(full_filename);
		free(full_filename);
		return vmc;
	}

	if (S_ISREG(st.st_mode)) {
		/* Regular file; could be .zip or .jar */
		return load_class_from_zip(path, file);
	}

	return NULL;
}

struct vm_class *load_class(const char *class_name)
{
	struct vm_class *result;
	char *filename;
	const char *classpath;

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
		char *classpath_element;

		n = strspn(classpath + i, ":");
		i += n;

		n = strcspn(classpath + i, ":");
		if (n == 0)
			continue;

		classpath_element = strndup(classpath + i, n);
		if (!classpath_element) {
			NOT_IMPLEMENTED;
			result = NULL;
			goto out_filename;
		}

		i += n;

		result = load_class_from_path_file(classpath_element, filename);
		free(classpath_element);

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
	class->class = vmc;

	return vmc;
}

struct vm_class *classloader_load_and_init(const char *class_name)
{
	struct vm_class *vmc;

	vmc = classloader_load(class_name);
	if (vmc) {
		if (vmc->state != VM_CLASS_INITIALIZED) {
			if (vm_class_run_clinit(vmc)) {
				NOT_IMPLEMENTED;
				return NULL;
			}
		}
	}

	return vmc;
}
