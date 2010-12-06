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

#ifndef CAFEBABE__ANNOTATIONS_ATTRIBUTE_H
#define CAFEBABE__ANNOTATIONS_ATTRIBUTE_H

#include <stdint.h>

#include "cafebabe/annotation.h"

/*
 * The RuntimeVisibleAnnotations and RuntimeInvisibleAnnotations attributes.
 *
 * See section 4.8.15 and 4.8.16 of The Java Virtual Machine Specification for details.
 */
struct cafebabe_annotations_attribute {
	uint16_t				num_annotations;
	struct cafebabe_annotation		*annotations;
};

int cafebabe_annotations_attribute_init(struct cafebabe_annotations_attribute *a, struct cafebabe_stream *s);
void cafebabe_annotations_attribute_deinit(struct cafebabe_annotations_attribute *a);
int cafebabe_read_annotations_attribute(const struct cafebabe_class *class, const struct cafebabe_attribute_array *attributes, struct cafebabe_annotations_attribute *annotations_attrib);

#endif
