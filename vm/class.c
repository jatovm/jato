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

#include <vm/vm.h>
#include <stdlib.h>

unsigned long is_object_instance_of(struct object *obj, struct object *type)
{
	if (!obj)
		return 0;

	NOT_IMPLEMENTED;
	//return isInstanceOf(type, obj->class);
	return 0;
}

void check_null(struct object *obj)
{
	if (!obj)
		abort();
}

void check_array(struct object *obj, unsigned int index)
{
	struct classblock *cb = CLASS_CB(obj->class);

	if (!IS_ARRAY(cb))
		abort();

	if (index >= ARRAY_LEN(obj))
		abort();
}

void check_cast(struct object *obj, struct object *type)
{
	if (!obj)
		return;

	NOT_IMPLEMENTED;
	//if (!isInstanceOf(type, obj->class))
	//	abort();
}

#include <cafebabe/class.h>
#include <cafebabe/method_info.h>
#include <vm/class.h>
#include <vm/method.h>

int vm_class_init(struct vm_class *vmc, const struct cafebabe_class *class)
{
	vmc->class = class;

	vmc->methods = malloc(sizeof(*vmc->methods) * class->methods_count);
	if (!vmc->methods) {
		NOT_IMPLEMENTED;
		return -1;
	}

	for (uint16_t i = 0; i < class->methods_count; ++i)
		vm_method_init(&vmc->methods[i], vmc, i);

	return 0;
}
