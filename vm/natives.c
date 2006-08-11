/*
 * Copyright (C) 2006  Pekka Enberg
 *
 * This file is released under the GPL version 2. Please refer to the file
 * LICENSE for details.
 */

#include <vm/natives.h>
#include <vm/system.h>

#include <string.h>

static void *lookup_method_ptr(struct vm_method *method_ptrs, const char *method_name)
{
	struct vm_method *method;

	for (method = method_ptrs; method->method_name; method++) {
		if (strcmp(method->method_name, method_name))
			continue;

		return method->method_ptr;
	}

	return NULL;
}

void *vm_lookup_native(struct vm_class *native_methods,
		       const char *class_name, const char *method_name)
{
	struct vm_class *class;

	for (class = native_methods; class->class_name; class++) {
		if (strcmp(class->class_name, class_name))
			continue;

		return lookup_method_ptr(class->method_ptrs, method_name);
	}

	return NULL;
}
