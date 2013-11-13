/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#ifndef CAFEBABE__CONSTANT_VALUE_ATTRIBUTE_H
#define CAFEBABE__CONSTANT_VALUE_ATTRIBUTE_H

#include <stdint.h>

#include "cafebabe/attribute_array.h"
#include "cafebabe/attribute_info.h"

/**
 * The ConstantValue attribute.
 *
 * See also section 4.7.2 of The Java Virtual Machine Specification.
 */
struct cafebabe_constant_value_attribute {
	uint16_t constant_value_index;
};

int cafebabe_constant_value_attribute_init(
	struct cafebabe_constant_value_attribute *a,
	struct cafebabe_stream *s);
void cafebabe_constant_value_attribute_deinit(
	struct cafebabe_constant_value_attribute *a);

#endif

