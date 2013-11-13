/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#ifndef CAFEBABE__ATTRIBUTE_ARRAY_H
#define CAFEBABE__ATTRIBUTE_ARRAY_H

#include <stdint.h>

struct cafebabe_attribute_info;
struct cafebabe_class;

struct cafebabe_attribute_array {
	uint16_t count;
	struct cafebabe_attribute_info *array;
};

int cafebabe_attribute_array_get(const struct cafebabe_attribute_array *array,
	const char *name, const struct cafebabe_class *c, unsigned int *r);

#endif
