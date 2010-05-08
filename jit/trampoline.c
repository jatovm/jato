/*
 * Copyright (c) 2005-2008  Pekka Enberg
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

#include "jit/cu-mapping.h"
#include "jit/emit-code.h"
#include "jit/exception.h"
#include "jit/compiler.h"

#include "vm/preload.h"
#include "lib/buffer.h"
#include "vm/class.h"
#include "vm/method.h"
#include "vm/natives.h"
#include "lib/string.h"
#include "vm/method.h"
#include "vm/die.h"
#include "vm/vm.h"
#include "vm/jni.h"
#include "vm/stack-trace.h"

#include <stdio.h>

static void *jit_jni_trampoline(struct compilation_unit *cu)
{
	const char *method_name, *class_name, *method_type;
	struct vm_method *method;
	void *ret;

	method = cu->method;

	class_name  = method->class->name;
	method_name = method->name;
	method_type = method->type;

	ret = vm_jni_lookup_method(class_name, method_name, method_type);
	if (ret) {
		struct buffer *buf;

		if (add_cu_mapping((unsigned long)ret, cu))
			return NULL;

		buf = alloc_exec_buffer();
		if (!buf)
			return NULL;

		emit_jni_trampoline(buf, method, ret);

		cu->native_ptr = buffer_ptr(buf);
		cu->is_compiled = true;

		return cu->native_ptr;
	}

	struct string *msg = alloc_str();
	if (!msg) {
		signal_new_exception(vm_java_lang_OutOfMemoryError, NULL);
		goto out;
	}

	str_printf(msg, "%s.%s%s", method->class->name, method->name,
		   method->type);

	if (strcmp(class_name, "VMThrowable") == 0)
		die("no native function found for %s", msg->value);

	signal_new_exception(vm_java_lang_UnsatisfiedLinkError, msg->value);
	free_str(msg);
out:
	return NULL;
}

static void *jit_java_trampoline(struct compilation_unit *cu)
{
	if (compile(cu)) {
		assert(exception_occurred() != NULL);

		return NULL;
	}

	if (add_cu_mapping((unsigned long)buffer_ptr(cu->objcode), cu) != 0) {
		signal_new_exception(vm_java_lang_OutOfMemoryError, NULL);
		return NULL;
	}

	return buffer_ptr(cu->objcode);
}

void *jit_magic_trampoline(struct compilation_unit *cu)
{
	struct vm_method *method = cu->method;
	void *ret;

	if (vm_method_is_static(method)) {
		/* This is for "invokestatic"... */
		if (vm_class_ensure_init(method->class))
			return NULL;
	}

	if (opt_trace_magic_trampoline)
		trace_magic_trampoline(cu);

	pthread_spin_lock(&cu->spinlock);

	if (cu->is_compiled)
		/* XXX: even if method is compiled we still might need
		 * to fixup some call sites. */
		ret = cu->native_ptr;
	else {
		if (vm_method_is_native(cu->method))
			ret = jit_jni_trampoline(cu);
		else
			ret = jit_java_trampoline(cu);

		shrink_compilation_unit(cu);
	}

	/*
	 * A method can be invoked by invokevirtual and invokespecial. For
	 * example, a public method p() in class A is normally invoked with
	 * invokevirtual but if a class B that extends A calls that
	 * method by "super.p()" we use invokespecial instead.
	 *
	 * Therefore, do fixup for direct call sites unconditionally and fixup
	 * vtables if method can be invoked via invokevirtual.
	 */

	if (ret && method_is_virtual(method))
		fixup_vtable(cu, ret);

	pthread_spin_unlock(&cu->spinlock);

	/*
	 * XXX: this must be done with cu->mutex unlocked because both
	 * fixup_static() and fixup_direct_calls() might need to lock
	 * on this compilation unit.
	 */
	if (ret) {
		fixup_direct_calls(method->trampoline, (unsigned long) ret);

		if (vm_method_is_static(cu->method))
			fixup_static(cu->method->class);
	}

	return ret;
}

struct jit_trampoline *build_jit_trampoline(struct compilation_unit *cu)
{
	struct jit_trampoline *trampoline;

	trampoline = alloc_jit_trampoline();
	if (!trampoline)
		return NULL;

	emit_trampoline(cu, jit_magic_trampoline, trampoline);
	add_cu_mapping((unsigned long) buffer_ptr(trampoline->objcode), cu);

	return trampoline;
}
