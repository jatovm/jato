/*
 * Copyright (c) 2009 Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "runtime/java_lang_VMClassLoader.h"

#include "jit/exception.h"

#include "vm/errors.h"
#include "vm/call.h"
#include "vm/class.h"
#include "vm/classloader.h"
#include "vm/object.h"
#include "vm/preload.h"
#include "vm/utf8.h"

#include <stdlib.h>
#include <stdio.h>

static const char *get_primitive_class_name(jint type)
{
	switch (type) {
	case 'Z':
		return "boolean";
	case 'B':
		return "byte";
	case 'C':
		return "char";
	case 'D':
		return "double";
	case 'F':
		return "float";
	case 'I':
		return "int";
	case 'J':
		return "long";
	case 'S':
		return "short";
	case 'V':
		return "void";
	default:
		die("unknown type: %d\n", type);
	}
}

jobject java_lang_VMClassLoader_getPrimitiveClass(jint type)
{
	const char *class_name = get_primitive_class_name(type);
	if (!class_name)
		return throw_npe();

	struct vm_class *class
		= classloader_load_primitive(class_name);

	if (!class)
		return NULL;

	vm_class_ensure_init(class);
	if (exception_occurred())
		return NULL;

	return class->object;
}

jobject java_lang_VMClassLoader_findLoadedClass(jobject classloader, jobject name)
{
	struct vm_class *vmc;
	char *c_name;

	c_name = vm_string_to_cstr(name);
	if (!c_name)
		return NULL;

	vmc = classloader_find_class(classloader, c_name);
	free(c_name);

	if (!vmc)
		return NULL;

	if (vm_class_ensure_object(vmc))
		return rethrow_exception();

	return vmc->object;
}

jobject java_lang_VMClassLoader_loadClass(jobject name, jboolean resolve)
{
	struct vm_class *vmc;
	char *c_name;

	c_name = vm_string_classname_to_cstr(name);
	if (!c_name)
		return NULL;

	vmc = classloader_load(NULL, c_name);
	free(c_name);
	if (!vmc)
		return NULL;

	if (vm_class_ensure_object(vmc))
		return NULL;

	if (resolve)
		java_lang_VMClassLoader_resolveClass(NULL, vmc->object);

	return vmc->object;
}

jobject java_lang_VMClassLoader_defineClass(jobject classloader, jobject name,
	jobject data, jint offset, jint len, jobject pd)
{
	struct vm_class *class;
	char *c_name;
	uint8_t *buf;

	buf = malloc(len);
	if (!buf)
		return throw_oom_error();

	for (jint i = 0; i < len; i++)
		buf[i] = array_get_field_byte(data, offset + i);

	if (name)
		c_name = vm_string_to_cstr(name);
	else
		c_name = strdup("unknown");

	class = vm_class_define(classloader, c_name, buf, len);
	free(buf);

	if (!class)
		return rethrow_exception();

	if (classloader_add_to_cache(classloader, class))
		return throw_oom_error();

	if (vm_class_ensure_object(class))
		return rethrow_exception();

	vm_call_method(vm_java_lang_Class_init, class->object, class, pd);
	if (exception_occurred())
		return rethrow_exception();

	return class->object;
}

void java_lang_VMClassLoader_resolveClass(jobject classloader, jobject clazz)
{
	/* FIXME */
}
