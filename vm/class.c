/*
 * Copyright (c) 2008 Saeed Siam
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

#include <jit/exception.h>
#include <jit/compiler.h>
#include <vm/string.h>
#include <vm/class.h>
#include <vm/die.h>
#include <vm/vm.h>
#include <stdlib.h>
#include <errno.h>

typedef void (*exception_init_fn)(struct object *, struct object *);

struct object *new_exception(char *class_name, char *message)
{
	struct object *message_str;
	exception_init_fn init;
	struct methodblock *mb;
	struct object *obj;
	Class *e_class;

	e_class = findSystemClass(class_name);
	if (!e_class)
		return NULL;

	obj = allocObject(e_class);
	if (!obj)
		return NULL;

	if (message == NULL)
		message_str = NULL;
	else
		message_str = Cstr2String(message);

	mb = lookupMethod(e_class, "<init>", "(Ljava/lang/String;)V");
	if (!mb)
		die("%s: constructor not found for class %s\n",
		    __func__, class_name);

	init = method_trampoline_ptr(mb);
	init(obj, message_str);

	return obj;
}

unsigned long is_object_instance_of(struct object *obj, struct object *type)
{
	if (!obj)
		return 0;

	return isInstanceOf(type, obj->class);
}

void check_array(struct object *obj, unsigned int index)
{
	unsigned int array_len;
	struct classblock *cb;
	char index_str[32];

	cb = CLASS_CB(obj->class);

	if (!IS_ARRAY(cb)) {
		signal_new_exception("java/lang/RuntimeException",
				     "object is not an array");
		goto throw;
	}

	array_len = ARRAY_LEN(obj);

	if (index < array_len)
		return;

	sprintf(index_str, "%d > %d", index, array_len - 1);
	signal_new_exception("java/lang/ArrayIndexOutOfBoundsException",
			     index_str);

 throw:
	throw_from_native(sizeof(struct object *) + sizeof(unsigned int));
}

void array_store_check(struct object *arrayref, struct object *obj)
{
	struct classblock *cb;
	struct string *str;
	int err;

	cb = CLASS_CB(arrayref->class);

	if (!IS_ARRAY(cb)) {
		signal_new_exception("java/lang/RuntimeException",
				     "object is not an array");
		goto throw;
	}

	if (obj == NULL || isInstanceOf(cb->element_class, obj->class))
		return;

	str = alloc_str();
	if (str == NULL) {
		err = -ENOMEM;
		goto error;
	}

	err = str_append(str, slash2dots(CLASS_CB(obj->class)->name));
	if (err)
		goto error;

	signal_new_exception("java/lang/ArrayStoreException", str->value);
	free_str(str);

 throw:
	throw_from_native(2 * sizeof(struct object *));
	return;

 error:
	if (str)
		free_str(str);

	if (err == -ENOMEM) /* TODO: throw OutOfMemoryError */
		die("%s: out of memory", __func__);

	die("%s: error %d", __func__, err);
}

void array_store_check_vmtype(struct object *arrayref, enum vm_type vm_type)
{
	/* TODO: Implement assignment compatibility checking described
	   in chapter "2.6.7 Assignment Conversion" of The Java VM
	   Specification - Second Edition. */
}

void check_cast(struct object *obj, struct object *type)
{
	struct string *str;
	int err;

	if (!obj || isInstanceOf(type, obj->class))
		return;

	if (exception_occurred())
		goto throw;

	str = alloc_str();
	if (str == NULL) {
		err = -ENOMEM;
		goto error;
	}

	err = str_append(str, slash2dots(CLASS_CB(obj->class)->name));
	if (err)
		goto error;

	err = str_append(str, " cannot be cast to ");
	if (err)
		goto error;

	err = str_append(str, slash2dots(CLASS_CB(type)->name));
	if (err)
		goto error;

	signal_new_exception("java/lang/ClassCastException", str->value);
	free_str(str);
 throw:
	throw_from_native(2 * sizeof(struct object *));
	return;

 error:
	if (str)
		free_str(str);

	if (err == -ENOMEM) /* TODO: throw OutOfMemoryError */
		die("%s: out of memory", __func__);

	die("%s: error %d", __func__, err);
}

void array_size_check(int size)
{
	if (size < 0) {
		signal_new_exception(
			"java/lang/NegativeArraySizeException", NULL);
		throw_from_native(sizeof(int));
	}
}

void multiarray_size_check(int n, ...)
{
	va_list ap;
	int i;

	va_start(ap, n);

	for (i = 0; i < n; i++) {
		if (va_arg(ap, int) >= 0)
			continue;

		signal_new_exception("java/lang/NegativeArraySizeException",
				     NULL);
		va_end(ap);
		throw_from_native(sizeof(int) * (n + 1));
		return;
	}

	va_end(ap);
	return;
}
