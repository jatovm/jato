/*
 * Copyright (c) 2009  Vegard Nossum
 *               2009  Tomasz Grabiec
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

#include <stdio.h>

#include "vm/die.h"
#include "vm/classloader.h"
#include "vm/preload.h"
#include "vm/class.h"

struct preload_entry {
	const char *name;
	struct vm_class **class;
};

struct vm_class *vm_java_lang_Object;
struct vm_class *vm_java_lang_Class;
struct vm_class *vm_java_lang_Cloneable;
struct vm_class *vm_java_lang_String;
struct vm_class *vm_java_lang_Throwable;
struct vm_class *vm_java_util_Properties;
struct vm_class *vm_java_lang_VMThrowable;
struct vm_class *vm_java_lang_StackTraceElement;
struct vm_class *vm_array_of_java_lang_StackTraceElement;
struct vm_class *vm_java_lang_Error;
struct vm_class *vm_java_lang_ArithmeticException;
struct vm_class *vm_java_lang_NullPointerException;
struct vm_class *vm_java_lang_NegativeArraySizeException;
struct vm_class *vm_java_lang_ClassCastException;
struct vm_class *vm_java_lang_NoClassDefFoundError;
struct vm_class *vm_java_lang_UnsatisfiedLinkError;
struct vm_class *vm_java_lang_ArrayIndexOutOfBoundsException;
struct vm_class *vm_java_lang_ArrayStoreException;
struct vm_class *vm_java_lang_RuntimeException;
struct vm_class *vm_java_lang_ExceptionInInitializerError;
struct vm_class *vm_java_lang_NoSuchFieldError;
struct vm_class *vm_java_lang_NoSuchMethodError;
struct vm_class *vm_boolean_class;
struct vm_class *vm_char_class;
struct vm_class *vm_float_class;
struct vm_class *vm_double_class;
struct vm_class *vm_byte_class;
struct vm_class *vm_short_class;
struct vm_class *vm_int_class;
struct vm_class *vm_long_class;

static const struct preload_entry preload_entries[] = {
	{ "java/lang/Object",		&vm_java_lang_Object },
	{ "java/lang/Class",		&vm_java_lang_Class },
	{ "java/lang/Cloneable",	&vm_java_lang_Cloneable },
	{ "java/lang/String",		&vm_java_lang_String },
	{ "java/lang/Throwable",	&vm_java_lang_Throwable },
	{ "java/util/Properties",	&vm_java_util_Properties },
	{ "java/lang/StackTraceElement", &vm_java_lang_StackTraceElement },
	{ "[Ljava/lang/StackTraceElement;", &vm_array_of_java_lang_StackTraceElement },
	{ "java/lang/VMThrowable",	&vm_java_lang_VMThrowable },
	{ "java/lang/ArithmeticException", &vm_java_lang_ArithmeticException },
	{ "java/lang/ArrayIndexOutOfBoundsException", &vm_java_lang_ArrayIndexOutOfBoundsException },
	{ "java/lang/ArrayStoreException", &vm_java_lang_ArrayStoreException },
	{ "java/lang/ClassCastException", &vm_java_lang_ClassCastException },
	{ "java/lang/Error",		&vm_java_lang_Error },
	{ "java/lang/ExceptionInInitializerError", &vm_java_lang_ExceptionInInitializerError },
	{ "java/lang/NegativeArraySizeException", &vm_java_lang_NegativeArraySizeException },
	{ "java/lang/NoClassDefFoundError", &vm_java_lang_NoClassDefFoundError },
	{ "java/lang/NullPointerException", &vm_java_lang_NullPointerException },
	{ "java/lang/RuntimeException",	&vm_java_lang_RuntimeException },
	{ "java/lang/UnsatisfiedLinkError", &vm_java_lang_UnsatisfiedLinkError },
	{ "java/lang/NoSuchFieldError", &vm_java_lang_NoSuchFieldError },
	{ "java/lang/NoSuchMethodError", &vm_java_lang_NoSuchMethodError },
};

static const struct preload_entry primitive_preload_entries[] = {
	{"Z", &vm_boolean_class},
	{"C", &vm_char_class},
	{"F", &vm_float_class},
	{"D", &vm_double_class},
	{"B", &vm_byte_class},
	{"S", &vm_short_class},
	{"I", &vm_int_class},
	{"J", &vm_long_class},
};

struct field_preload_entry {
	struct vm_class **class;
	const char *name;
	const char *type;
	struct vm_field **field;
};

struct vm_field *vm_java_lang_Class_vmdata;
struct vm_field *vm_java_lang_String_offset;
struct vm_field *vm_java_lang_String_count;
struct vm_field *vm_java_lang_String_value;
struct vm_field *vm_java_lang_Throwable_detailMessage;
struct vm_field *vm_java_lang_VMThrowable_vmdata;

static const struct field_preload_entry field_preload_entries[] = {
	{ &vm_java_lang_Class, "vmdata", "Ljava/lang/Object;", &vm_java_lang_Class_vmdata },
	{ &vm_java_lang_String, "offset", "I",	&vm_java_lang_String_offset },
	{ &vm_java_lang_String, "count", "I",	&vm_java_lang_String_count },
	{ &vm_java_lang_String, "value", "[C",	&vm_java_lang_String_value },
	{ &vm_java_lang_Throwable, "detailMessage", "Ljava/lang/String;", &vm_java_lang_Throwable_detailMessage },
	{ &vm_java_lang_VMThrowable, "vmdata", "Ljava/lang/Object;", &vm_java_lang_VMThrowable_vmdata },
};

struct method_preload_entry {
	struct vm_class **class;
	const char *name;
	const char *type;
	struct vm_method **method;
};

struct vm_method *vm_java_util_Properties_setProperty;
struct vm_method *vm_java_lang_Throwable_initCause;
struct vm_method *vm_java_lang_Throwable_getCause;
struct vm_method *vm_java_lang_Throwable_toString;
struct vm_method *vm_java_lang_Throwable_getStackTrace;
struct vm_method *vm_java_lang_StackTraceElement_getFileName;
struct vm_method *vm_java_lang_StackTraceElement_getClassName;
struct vm_method *vm_java_lang_StackTraceElement_getMethodName;
struct vm_method *vm_java_lang_StackTraceElement_getLineNumber;
struct vm_method *vm_java_lang_StackTraceElement_isNativeMethod;
struct vm_method *vm_java_lang_StackTraceElement_equals;

static const struct method_preload_entry method_preload_entries[] = {
	{
		&vm_java_util_Properties,
		"setProperty",
		"(Ljava/lang/String;Ljava/lang/String;)Ljava/lang/Object;",
		&vm_java_util_Properties_setProperty,
	},
	{
		&vm_java_lang_Throwable,
		"initCause",
		"(Ljava/lang/Throwable;)Ljava/lang/Throwable;",
		&vm_java_lang_Throwable_initCause,
	},
	{
		&vm_java_lang_Throwable,
		"getCause",
		"()Ljava/lang/Throwable;",
		&vm_java_lang_Throwable_getCause,
	},
	{
		&vm_java_lang_Throwable,
		"getStackTrace",
		"()[Ljava/lang/StackTraceElement;",
		&vm_java_lang_Throwable_getStackTrace,
	},
	{
		&vm_java_lang_Throwable,
		"toString",
		"()Ljava/lang/String;",
		&vm_java_lang_Throwable_toString,
	},
	{
		&vm_java_lang_StackTraceElement,
		"getFileName",
		"()Ljava/lang/String;",
		&vm_java_lang_StackTraceElement_getFileName,
	},
	{
		&vm_java_lang_StackTraceElement,
		"getClassName",
		"()Ljava/lang/String;",
		&vm_java_lang_StackTraceElement_getClassName,
	},
	{
		&vm_java_lang_StackTraceElement,
		"getMethodName",
		"()Ljava/lang/String;",
		&vm_java_lang_StackTraceElement_getMethodName,
	},
	{
		&vm_java_lang_StackTraceElement,
		"getLineNumber",
		"()I",
		&vm_java_lang_StackTraceElement_getLineNumber,
	},
	{
		&vm_java_lang_StackTraceElement,
		"isNativeMethod",
		"()Z",
		&vm_java_lang_StackTraceElement_isNativeMethod,
	},
	{
		&vm_java_lang_StackTraceElement,
		"equals",
		"(Ljava/lang/Object;)Z",
		&vm_java_lang_StackTraceElement_equals,
	},
};

int preload_vm_classes(void)
{
	unsigned int array_size;

	for (unsigned int i = 0; i < ARRAY_SIZE(preload_entries); ++i) {
		const struct preload_entry *pe = &preload_entries[i];

		struct vm_class *class = classloader_load(pe->name);
		if (!class) {
			NOT_IMPLEMENTED;
			return 1;
		}

		*pe->class = class;
	}

	array_size = ARRAY_SIZE(primitive_preload_entries);
	for (unsigned int i = 0; i < array_size; ++i) {
		const struct preload_entry *pe = &primitive_preload_entries[i];

		struct vm_class *class = classloader_load_primitive(pe->name);
		if (!class) {
			NOT_IMPLEMENTED;
			return 1;
		}

		*pe->class = class;
	}

	for (unsigned int i = 0; i < ARRAY_SIZE(field_preload_entries); ++i) {
		const struct field_preload_entry *pe
			= &field_preload_entries[i];

		struct vm_field *field = vm_class_get_field(*pe->class,
			pe->name, pe->type);
		if (!field) {
			NOT_IMPLEMENTED;
			return 1;
		}

		*pe->field = field;
	}

	for (unsigned int i = 0; i < ARRAY_SIZE(method_preload_entries); ++i) {
		const struct method_preload_entry *me
			= &method_preload_entries[i];

		struct vm_method *method = vm_class_get_method(*me->class,
			me->name, me->type);
		if (!method) {
			warn("preload of %s.%s%s failed", (*me->class)->name,
			     me->name, me->type);
			NOT_IMPLEMENTED;
			return 1;
		}

		*me->method = method;
	}

	return 0;
}
