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
