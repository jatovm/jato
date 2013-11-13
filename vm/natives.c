/*
 * Copyright (c) 2006  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "vm/natives.h"
#include "vm/system.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

struct vm_method {
	const char *method_name;
	void *method_ptr;
};

struct vm_class {
	const char *class_name;
	unsigned char nr_ptrs;
	struct vm_method *method_ptrs;
};

static struct vm_class *native_methods;
static unsigned long nr_natives;

static struct vm_class *lookup_class(const char *class_name)
{
	unsigned long i;

	for (i = 0; i < nr_natives; i++) {
		struct vm_class *class = native_methods + i;

		if (!strcmp(class->class_name, class_name))
			return class;
	}

	return NULL;
}

static void *lookup_method(struct vm_class *class, const char *method_name)
{
	int i;

	for (i = 0; i < class->nr_ptrs; i++) {
		struct vm_method *method = class->method_ptrs + i;

		if (!strcmp(method->method_name, method_name))
			return method->method_ptr;
	}

	return NULL;
}

void *vm_lookup_native(const char *class_name, const char *method_name)
{
	struct vm_class *class;

	class = lookup_class(class_name);
	if (!class)
		return NULL;

	return lookup_method(class, method_name);
}

static struct vm_class *new_class(const char *class_name)
{
	unsigned long new_size;
	struct vm_class *class;

	nr_natives++;
	new_size = nr_natives * sizeof(struct vm_class);
	native_methods = realloc(native_methods, new_size);
	if (!native_methods)
		return NULL;

	class = native_methods + nr_natives-1;
	class->class_name = class_name;
	class->method_ptrs = NULL;
	class->nr_ptrs = 0;

	return class;
}

static int insert_native(struct vm_class *class, const char *method_name, void *ptr)
{
	struct vm_method *method;
	unsigned long new_size;

	class->nr_ptrs++;
	new_size = class->nr_ptrs * sizeof(struct vm_method);
	class->method_ptrs = realloc(class->method_ptrs, new_size);
	if (!class->method_ptrs)
		return -ENOMEM;

	method = class->method_ptrs + class->nr_ptrs-1;
	method->method_name = method_name;
	method->method_ptr  = ptr;;

	return 0;
}

int vm_register_native(struct vm_native *native)
{
	struct vm_class *class;
	int err = 0;

	class = lookup_class(native->class_name);
	if (!class)
		class = new_class(native->class_name);

	if (!class) {
		err = -ENOMEM;
		goto out;
	}

	err = insert_native(class, native->method_name, native->ptr);
  out:
	return err;
}

void vm_unregister_natives(void)
{
	unsigned long i;

	for (i = 0; i < nr_natives; i++) {
		struct vm_class *class = native_methods + i;

		free(class->method_ptrs);
	}
	free(native_methods);
	native_methods = NULL;
	nr_natives = 0;
}
