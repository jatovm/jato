/*
 * Copyright (c) 2009 Tomasz Grabiec
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
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

void java_lang_VMRuntime_runFinalization(void)
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

void native_vmruntime_trace_method_calls(jboolean on)
{
}
