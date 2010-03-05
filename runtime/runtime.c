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

#include "runtime/runtime.h"
#include "jit/exception.h"
#include "vm/preload.h"
#include "vm/object.h"

#include <stdlib.h>
#include <stdio.h>

void native_vmruntime_exit(int status)
{
	/* XXX: exit gracefully */
	exit(status);
}

void native_vmruntime_run_finalization_for_exit(void)
{
}

struct vm_object *native_vmruntime_maplibraryname(struct vm_object *name)
{
	struct vm_object *result;
	char *str;

	if (!name) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return NULL;
	}

	str = vm_string_to_cstr(name);
	if (!str) {
		signal_new_exception(vm_java_lang_OutOfMemoryError, NULL);
		return NULL;
	}

	char *result_str = NULL;

	if (asprintf(&result_str, "lib%s.so", str) < 0)
		die("asprintf");

	free(str);

	if (!result_str) {
		signal_new_exception(vm_java_lang_OutOfMemoryError, NULL);
		return NULL;
	}

	result = vm_object_alloc_string_from_c(result_str);
	free(result_str);

	return result;
}

int native_vmruntime_native_load(struct vm_object *name,
				 struct vm_object *classloader)
{
	char *name_str;
	int result;

	if (!name) {
		signal_new_exception(vm_java_lang_NullPointerException, NULL);
		return 0;
	}

	name_str = vm_string_to_cstr(name);
	if (!name_str) {
		signal_new_exception(vm_java_lang_OutOfMemoryError, NULL);
		return 0;
	}

	result = vm_jni_load_object(name_str, classloader);
	free(name_str);

	return result == 0;
}
