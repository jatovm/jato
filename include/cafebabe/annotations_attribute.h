/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2010  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
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
