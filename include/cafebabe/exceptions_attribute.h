/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2010  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#ifndef CAFEBABE__EXCEPTIONS_ATTRIBUTE_H
#define CAFEBABE__EXCEPTIONS_ATTRIBUTE_H

#include <stdint.h>

#include "cafebabe/attribute_array.h"
#include "cafebabe/attribute_info.h"

/**
 * The Exceptions attribute.
 *
 * See also section 4.7.4 of the Java Virtual Machine Specification.
 */
struct cafebabe_exceptions_attribute {
	uint16_t number_of_exceptions;
	uint16_t *exceptions;
};

int cafebabe_exceptions_attribute_init(struct cafebabe_exceptions_attribute *a,
				       struct cafebabe_stream *s);
void cafebabe_exceptions_attribute_deinit(struct cafebabe_exceptions_attribute *a);
int cafebabe_read_exceptions_attribute(const struct cafebabe_class *class,
					     const struct cafebabe_attribute_array *attributes,
					     struct cafebabe_exceptions_attribute *exceptions_attrib);

#endif
