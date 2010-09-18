/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2010  Pekka Enberg
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

#include <stdio.h>
#include <stdlib.h>

#include "cafebabe/inner_classes_attribute.h"
#include "cafebabe/attribute_info.h"
#include "cafebabe/stream.h"

int
cafebabe_inner_classes_attribute_init(struct cafebabe_inner_classes_attribute *a, struct cafebabe_stream *s)
{
	if (cafebabe_stream_read_uint16(s, &a->number_of_classes))
		goto out;

	a->inner_classes = cafebabe_stream_malloc(s, sizeof(*a->inner_classes) * a->number_of_classes);
	if (!a->inner_classes)
		goto out;

	for (uint16_t i = 0; i < a->number_of_classes; ++i) {
		struct cafebabe_inner_class *e = &a->inner_classes[i];

		if (cafebabe_stream_read_uint16(s, &e->inner_class_info_index))
			goto out_free;

		if (cafebabe_stream_read_uint16(s, &e->outer_class_info_index))
			goto out_free;

		if (cafebabe_stream_read_uint16(s, &e->inner_name_index))
			goto out_free;

		if (cafebabe_stream_read_uint16(s, &e->inner_class_access_flags))
			goto out_free;
	}
	return 0;

out_free:
	free(a->inner_classes);
out:
	return 1;
}

void
cafebabe_inner_classes_attribute_deinit(struct cafebabe_inner_classes_attribute *a)
{
	free(a->inner_classes);
}
