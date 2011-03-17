/*
 * Copyright (c) 2009 Tomasz Grabiec
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

#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "jit/disassemble.h"

#include "vm/die.h"
#include "vm/jni.h"
#include "vm/stack-trace.h"
#include "vm/gc.h"

#include "lib/hash-map.h"
#include "lib/string.h"

struct jni_object {
	void *handle; /* returned by dlopen() */
	struct vm_object *classloader;
};

static unsigned long jni_object_hash(const void *key, unsigned long size)
{
	unsigned long a;

	a = (unsigned long) key;
	return ((a >> 24) ^ (a >> 16) ^ (a >> 8) ^ a) % size;
}

static int jni_object_compare(const void *key1, const void *key2)
{
	return (long) key1 - (long) key2;
}

struct hash_map *jni_objects;

static char *vm_jni_get_mangled_name(const char *name)
{
	struct string *str;
	char *result;
	int err;
	int i;

	str = alloc_str();
	if (!str)
		return NULL;

	result = NULL;

	for (i = 0; name[i]; i++) {
		if (name[i] == '_')
			err = str_append(str, "_1");
		else if (name[i] == ';')
			err = str_append(str, "_2");
		else if (name[i] == '[')
			err = str_append(str, "_3");
		else if (name[i] == '/')
			err = str_append(str, "_");
		else if (name[i] & 0x80) {
			/*
			 * FIXME: Unicode characters should be transformed to "_0xxxx"
			 */
			die("unicode characters are not supported");
		} else
			err = str_append(str, "%c", (char)name[i]);

		if (err)
			goto out;
	}

	result = strdup(str->value);

 out:
	free_str(str);

	return result;
}

static int vm_jni_add_object(void *handle, struct vm_object *classloader)
{
	struct jni_object *object;

	if (hash_map_contains(jni_objects, handle))
		return 0;

	object = malloc(sizeof(*object));
	if (!object)
		return -ENOMEM;

	object->handle = handle;
	object->classloader = classloader;

	if (hash_map_put(jni_objects, handle, object)) {
		free(object);
		return -1;
	}

	return 0;
}

void vm_jni_init(void)
{
	jni_objects = alloc_hash_map(100, jni_object_hash, jni_object_compare);
	if (!jni_objects)
		error("failed to create jni_objects hash map");
}

typedef jint onload_fn(JavaVM *, void *);

int vm_jni_load_object(const char *name, struct vm_object *classloader)
{
	void *handle;

	handle = dlopen(name, RTLD_NOW);
	if (!handle)
		return -1;

	if (vm_jni_add_object(handle, classloader)) {
		dlclose(handle);
		return -ENOMEM;
	}

	onload_fn *onload = dlsym(handle, "JNI_OnLoad");
	if (!onload)
		return 0;

	vm_enter_jni(__builtin_frame_address(0), NULL,
		     (unsigned long) &&call_site);
 call_site:

	onload(vm_jni_get_current_java_vm(), NULL);
	vm_leave_jni();

	return 0;
}

static void *vm_jni_lookup_symbol(const char *symbol_name)
{
	struct hash_map_entry *this;

	hash_map_for_each_entry(this, jni_objects) {
		struct jni_object *object;
		void *addr;

		object = this->value;

		addr = dlsym(object->handle, symbol_name);
		if (addr)
			return addr;
	}

	return NULL;
}

static char *get_method_args(const char *type)
{
	char *result, *end;

	if (type[0] != '(')
		return NULL;

	result = strdup(type + 1);

	end = index(result, ')');
	if (!end) {
		free(result);
		return NULL;
	}

	*end = 0;

	return result;
}

void *vm_jni_lookup_method(const char *class_name, const char *method_name,
			   const char *method_type)
{
	char *mangled_class_name;
	char *mangled_method_name;
	char *mangled_method_type;
	char *symbol_name;
	void *sym_addr;
	char *method_args;

	method_args = get_method_args(method_type);

	mangled_class_name = vm_jni_get_mangled_name(class_name);
	mangled_method_name = vm_jni_get_mangled_name(method_name);
	mangled_method_type = vm_jni_get_mangled_name(method_args);

	symbol_name = NULL;
	if (asprintf(&symbol_name, "Java_%s_%s__%s", mangled_class_name,
		 mangled_method_name, mangled_method_type) < 0)
		die("asprintf");

	sym_addr = vm_jni_lookup_symbol(symbol_name);
	if (sym_addr)
		goto out;

	if (asprintf(&symbol_name, "Java_%s_%s", mangled_class_name,
		 mangled_method_name) < 0)
		die("asprintf");

	sym_addr = vm_jni_lookup_symbol(symbol_name);

 out:
	free(method_args);
	free(mangled_method_name);
	free(mangled_class_name);
	free(mangled_method_type);
	free(symbol_name);

	return sym_addr;
}
