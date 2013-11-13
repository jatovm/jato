/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#ifndef CAFEBABE__ATTRIBUTE_INFO_H
#define CAFEBABE__ATTRIBUTE_INFO_H

#include <stdint.h>

struct cafebabe_stream;

/**
 * An attribute.
 *
 * See also section 4.7 of The Java Virtual Machine Specification.
 */
struct cafebabe_attribute_info {
	uint16_t attribute_name_index;
	uint32_t attribute_length;
	uint8_t *info;
};

int cafebabe_attribute_info_init(struct cafebabe_attribute_info *a,
	struct cafebabe_stream *s);
void cafebabe_attribute_info_deinit(struct cafebabe_attribute_info *a);

#endif
