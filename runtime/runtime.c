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
#include "vm/gc.h"
#include "vm/errors.h"

#include "../boehmgc/include/gc.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

jlong native_vmruntime_free_memory(void)
{
	return GC_get_free_bytes();
}

jlong native_vmruntime_total_memory(void)
{
	return GC_get_heap_size();
}

jlong native_vmruntime_max_memory(void)
{
	return max_heap_size;
}

int native_vmruntime_available_processors(void)
{
	long ret;

	ret = sysconf(_SC_NPROCESSORS_ONLN);
	if (ret < 0) {
		warn("_SC_NPROCESSORS_ONLN sysconf failed");
		ret = 1;
	}

	return ret;
}

void native_vmruntime_gc(void)
{
	/* Nothing to do */
}

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

	if (!name)
		return throw_npe();

	str = vm_string_to_cstr(name);
	if (!str)
		return throw_oom_error();

	char *result_str = NULL;

	if (asprintf(&result_str, "lib%s.so", str) < 0)
		die("asprintf");

	free(str);

	if (!result_str)
		return throw_oom_error();

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
		throw_npe();
		return 0;
	}

	name_str = vm_string_to_cstr(name);
	if (!name_str) {
		throw_oom_error();
		return 0;
	}

	result = vm_jni_load_object(name_str, classloader);
	free(name_str);

	return result == 0;
}

void native_vmruntime_trace_instructions(jboolean on)
{
}
