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

#include <jit/cu-mapping.h>
#include <jit/emit-code.h>
#include <jit/exception.h>
#include <jit/compiler.h>

#include <vm/buffer.h>
#include <vm/class.h>
#include <vm/method.h>
#include <vm/natives.h>
#include <vm/string.h>
#include <vm/method.h>
#include <vm/buffer.h>
#include <vm/die.h>
#include <vm/vm.h>

static void *jit_native_trampoline(struct compilation_unit *cu)
{
	const char *method_name, *class_name;
	struct vm_method *method;
	struct string *msg;
	void *ret;

	method = cu->method;
	class_name  = method->class->name;
	method_name = method->name;

	ret = vm_lookup_native(class_name, method_name);
	if (ret) {
		add_cu_mapping((unsigned long)ret, cu);
		return ret;
	}

	msg = alloc_str();
	if (!msg)
		/* TODO: signal OutOfMemoryError */
		die("%s: out of memory\n", __func__);

	str_printf(msg, "%s.%s%s", CLASS_CB(method->class)->name, method->name,
		   method->type);

	if (strcmp(class_name, "VMThrowable") == 0)
		die("no native function found for %s", msg->value);

	signal_new_exception("java/lang/UnsatisfiedLinkError", msg->value);
	free_str(msg);

	return NULL;
}

static void *jit_java_trampoline(struct compilation_unit *cu)
{
	if (cu->is_compiled)
		return buffer_ptr(cu->objcode);

	if (compile(cu)) {
		assert(exception_occurred() != NULL);

		return NULL;
	}

	if (add_cu_mapping((unsigned long)buffer_ptr(cu->objcode), cu) != 0)
		/* TODO: throw OutOfMemoryError */
		die("%s: out of memory", __func__);

	return buffer_ptr(cu->objcode);
}

void *jit_magic_trampoline(struct compilation_unit *cu)
{
	struct vm_method *method = cu->method;
	void *ret;

	pthread_mutex_lock(&cu->mutex);

	if (opt_trace_magic_trampoline)
		trace_magic_trampoline(cu);

	if (vm_method_is_native(cu->method))
		ret = jit_native_trampoline(cu);
	else
		ret = jit_java_trampoline(cu);

	/*
	 * A method can be invoked by invokevirtual and invokespecial. For
	 * example, a public method p() in class A is normally invoked with
	 * invokevirtual but if a class B that extends A calls that
	 * method by "super.p()" we use invokespecial instead.
	 *
	 * Therefore, do fixup for direct call sites unconditionally and fixup
	 * vtables if method can be invoked via invokevirtual.
	 */
	if (cu->is_compiled)
		fixup_direct_calls(method->trampoline, (unsigned long) ret);

	pthread_mutex_unlock(&cu->mutex);

	return ret;
}

struct jit_trampoline *build_jit_trampoline(struct compilation_unit *cu)
{
	struct jit_trampoline *trampoline;
	
	trampoline = alloc_jit_trampoline();
	if (trampoline)
		emit_trampoline(cu, jit_magic_trampoline, trampoline);
	return trampoline;
}
