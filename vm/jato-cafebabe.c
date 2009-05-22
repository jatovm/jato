/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#define _GNU_SOURCE

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cafebabe/access.h"
#include "cafebabe/attribute_info.h"
#include "cafebabe/class.h"
#include "cafebabe/constant_pool.h"
#include "cafebabe/error.h"
#include "cafebabe/field_info.h"
#include "cafebabe/method_info.h"
#include "cafebabe/stream.h"

#include "jit/compiler.h"

#include "vm/class.h"
#include "vm/classloader.h"
#include "vm/method.h"
#include "vm/natives.h"
#include "vm/signal.h"
#include "vm/vm.h"

char *exe_name;

static void native_vmruntime_exit(int status)
{
	NOT_IMPLEMENTED;

	exit(status);
}

static void native_vmsystem_arraycopy(struct object *src, int src_start,
	struct object *dest, int dest_start, int len)
{
	NOT_IMPLEMENTED;
}

/*
 * This stub is needed by java.lang.VMThrowable constructor to work. It should
 * return java.lang.VMState instance, or null in which case no stack trace will
 * be printed by printStackTrace() method.
 */
static struct object *
native_vmthrowable_fill_in_stack_trace(struct object *object)
{
	NOT_IMPLEMENTED;

	return NULL;
}

static void jit_init_natives(void)
{
	vm_register_native("java/lang/VMRuntime", "exit",
		&native_vmruntime_exit);
	vm_register_native("jvm/TestCase", "exit",
		&native_vmruntime_exit);
	vm_register_native("java/lang/VMSystem", "arraycopy",
		&native_vmsystem_arraycopy);
	vm_register_native("java/lang/VMThrowable", "fillInStackTrace",
		&native_vmthrowable_fill_in_stack_trace);
}

static void native_Test_printf(int x)
{
	printf("%d\n", x);
}

int
main(int argc, char *argv[])
{
	exe_name = argv[0];

	if (argc != 2) {
		fprintf(stderr, "usage: %s CLASS\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	jit_init_natives();
	vm_register_native("Test", "printf", &native_Test_printf);

	const char *classname = argv[1];
	struct vm_class *vmc = classloader_load(classname);
	if (!vmc) {
		fprintf(stderr, "error: %s: could not load\n", classname);
		goto out;
	}

	unsigned int main_method_index;
	if (cafebabe_class_get_method(vmc->class,
		"main", "([Ljava/lang/String;)V", &main_method_index))
	{
		fprintf(stderr, "error: %s: no main method\n", classname);
		goto out;
	}

	struct vm_method *main_method = &vmc->methods[main_method_index];

	if (!vm_method_is_static(main_method)) {
		fprintf(stderr, "errror: %s: main method not static\n",
			classname);
		goto out;
	}

	void (*main_method_trampoline)(void)
		= vm_method_trampoline_ptr(main_method);
	main_method_trampoline();

	return EXIT_SUCCESS;

out:
	return EXIT_FAILURE;
}
