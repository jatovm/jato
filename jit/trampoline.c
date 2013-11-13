/*
 * Copyright (c) 2005-2010  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#include "arch/memory.h"
#include "arch/debug.h"

#include "jit/compiler.h"
#include "jit/cu-mapping.h"
#include "jit/emit-code.h"
#include "jit/exception.h"
#include "jit/debug.h"

#include "vm/stack-trace.h"
#include "vm/natives.h"
#include "vm/preload.h"
#include "vm/method.h"
#include "vm/method.h"
#include "vm/class.h"
#include "vm/die.h"
#include "vm/jni.h"
#include "vm/vm.h"
#include "vm/errors.h"

#include "lib/buffer.h"
#include "lib/string.h"

#include <stdio.h>

static void *jit_jni_trampoline(struct compilation_unit *cu)
{
	struct vm_method *method = cu->method;
	struct buffer *buf;
	void *target;

	target = vm_jni_lookup_method(method->class->name, method->name, method->type);
	if (!target) {
		signal_new_exception(vm_java_lang_UnsatisfiedLinkError, "%s.%s%s",
				     method->class->name, method->name, method->type);
		return rethrow_exception();
	}

	if (add_cu_mapping((unsigned long)target, cu))
		return NULL;

	buf = alloc_exec_buffer();
	if (!buf)
		return NULL;

	emit_jni_trampoline(buf, method, target);

	cu->entry_point = buffer_ptr(buf);

	return cu->entry_point;
}

static void *jit_java_trampoline(struct compilation_unit *cu)
{
	int err;

	err = compile(cu);
	if (err) {
		assert(exception_occurred() != NULL);
		return NULL;
	}

	err = add_cu_mapping((unsigned long)cu_entry_point(cu), cu);
	if (err)
		return throw_oom_error();

	return cu_entry_point(cu);
}

void *jit_magic_trampoline(struct compilation_unit *cu)
{
	struct vm_method *method = cu->method;
	unsigned long state;
	void *ret;

	if (opt_debug_stack)
		check_stack_align(method);

	if (opt_trace_magic_trampoline)
		trace_magic_trampoline(cu);

	if (vm_method_is_static(method)) {
		/* This is for "invokestatic"... */
		if (vm_class_ensure_init(method->class))
			return rethrow_exception();
	}

	state = compilation_unit_get_state(cu);

	if (state == COMPILATION_STATE_COMPILED) {
		ret = cu_entry_point(cu);
		goto out_fixup;
	}

	pthread_mutex_lock(&cu->compile_mutex);

	if (cu->state == COMPILATION_STATE_COMPILED) {
		ret = cu_entry_point(cu);
		goto out_unlock_fixup;
	}

	assert(cu->state == COMPILATION_STATE_INITIAL);

	cu->state = COMPILATION_STATE_COMPILING;

	if (vm_method_is_native(cu->method))
		ret = jit_jni_trampoline(cu);
	else
		ret = jit_java_trampoline(cu);

	if (ret)
		cu->state = COMPILATION_STATE_COMPILED;
	else
		cu->state = COMPILATION_STATE_INITIAL;

	shrink_compilation_unit(cu);

out_unlock_fixup:
	pthread_mutex_unlock(&cu->compile_mutex);

out_fixup:
	if (!ret)
		return rethrow_exception();

	fixup_direct_calls(method->trampoline, (unsigned long) ret);
	return ret;
}

struct jit_trampoline *build_jit_trampoline(struct compilation_unit *cu)
{
	struct jit_trampoline *ret;

	ret = alloc_jit_trampoline();
	if (!ret)
		return NULL;

	emit_trampoline(cu, jit_magic_trampoline, ret);
	add_cu_mapping((unsigned long) buffer_ptr(ret->objcode), cu);

	return ret;
}

void jit_no_such_method_stub(void)
{
	signal_new_exception(vm_java_lang_NoSuchMethodError, NULL);
}
