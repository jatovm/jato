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

#include <jit/compiler.h>
#include <vm/buffer.h>
#include <vm/natives.h>
#include <vm/vm.h>
#include <arch/emit-code.h>

static void *jit_native_trampoline(struct compilation_unit *cu)
{
	struct methodblock *method = cu->method;
	const char *method_name, *class_name;

	class_name  = CLASS_CB(method->class)->name;
	method_name = method->name;

	return vm_lookup_native(class_name, method_name);
}

static void *jit_java_trampoline(struct compilation_unit *cu)
{
	if (!cu->is_compiled)
		compile(cu);

	return buffer_ptr(cu->objcode);
}

void *jit_magic_trampoline(struct compilation_unit *cu)
{
	void *ret;

	pthread_mutex_lock(&cu->mutex);

	if (cu->method->access_flags & ACC_NATIVE)
		ret = jit_native_trampoline(cu);
	else
		ret = jit_java_trampoline(cu);

	pthread_mutex_unlock(&cu->mutex);

	return ret;
}

struct jit_trampoline *build_jit_trampoline(struct compilation_unit *cu)
{
	struct jit_trampoline *trampoline;
	
	trampoline = alloc_jit_trampoline();
	if (trampoline)
		emit_trampoline(cu, jit_magic_trampoline, trampoline->objcode);
	return trampoline;
}
