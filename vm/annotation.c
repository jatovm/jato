/*
 * Copyright (c) 2010 Pekka Enberg
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

#include "vm/annotation.h"

#include <stdlib.h>
#include <string.h>

#include "cafebabe/annotation.h"
#include "cafebabe/class.h"
#include "vm/gc.h"

struct vm_annotation *vm_annotation_parse(const struct cafebabe_class *klass, struct cafebabe_annotation *annotation)
{
	const struct cafebabe_constant_info_utf8 *type;
	struct vm_annotation *vma;

        if (cafebabe_class_constant_get_utf8(klass, annotation->type_index, &type))
                return NULL;

	vma = calloc(1, sizeof *vma);
	if (!vma)
		return NULL;

	vma->type = strndup((char *) type->bytes, type->length);
	if (!vma->type)
		goto out_free;

	return vma;
out_free:
	free(vma);

	return NULL;
}

void vm_annotation_free(struct vm_annotation *vma)
{
	free(vma->type);
}
