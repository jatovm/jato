/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2010  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#ifndef CAFEBABE__INNER_CLASSES_ATTRIBUTE_H
#define CAFEBABE__INNER_CLASSES_ATTRIBUTE_H

#include <stdint.h>

#include "cafebabe/attribute_array.h"
#include "cafebabe/attribute_info.h"

struct cafebabe_inner_class {
	uint16_t inner_class_info_index;
	uint16_t outer_class_info_index;
	uint16_t inner_name_index;
	uint16_t inner_class_access_flags;
};

/**
 * The InnerClasses attribute.
 *
 * See also section 4.7.5 of the Java Virtual Machine Specification.
 */
struct cafebabe_inner_classes_attribute {
	uint16_t number_of_classes;
	struct cafebabe_inner_class *inner_classes;
};

int cafebabe_inner_classes_attribute_init(struct cafebabe_inner_classes_attribute *a, struct cafebabe_stream *s);
void cafebabe_inner_classes_attribute_deinit(struct cafebabe_inner_classes_attribute *a);
int cafebabe_read_inner_classes_attribute(const struct cafebabe_class *class,
					     const struct cafebabe_attribute_array *attributes,
					     struct cafebabe_inner_classes_attribute *inner_classes_attrib);

#endif
