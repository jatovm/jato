/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2010  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#ifndef CAFEBABE__ENCLOSING_METHOD_ATTRIBUTE_H
#define CAFEBABE__ENCLOSING_METHOD_ATTRIBUTE_H

#include <stdint.h>

#include "cafebabe/attribute_array.h"
#include "cafebabe/attribute_info.h"

/**
 * The EnclosingMethod attribute.
 *
 * See Section 4.8.6 of the Java Virtual Machine Specification for details.
 */
struct cafebabe_enclosing_method_attribute {
	uint16_t		class_index;
	uint16_t		method_index;
};

int cafebabe_read_enclosing_method_attribute(const struct cafebabe_class *class,
					     const struct cafebabe_attribute_array *attributes,
					     struct cafebabe_enclosing_method_attribute *enclosing_method_attrib);

#endif /* CAFEBABE__ENCLOSING_METHOD_ATTRIBUTE_H */
