/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#ifndef CAFEBABE__CODE_ATTRIBUTE_H
#define CAFEBABE__CODE_ATTRIBUTE_H

#include <stdint.h>

#include "cafebabe/attribute_array.h"
#include "cafebabe/attribute_info.h"

struct cafebabe_code_attribute_exception {
	uint16_t start_pc;
	uint16_t end_pc;
	uint16_t handler_pc;
	uint16_t catch_type;
};

/**
 * The Code attribute.
 *
 * See also section 4.7.3 of The Java Virtual Machine Specification.
 */
struct cafebabe_code_attribute {
	uint16_t max_stack;
	uint16_t max_locals;

	uint32_t code_length;
	uint8_t *code;

	uint16_t exception_table_length;
	struct cafebabe_code_attribute_exception *exception_table;

	struct cafebabe_attribute_array attributes;
};

int cafebabe_code_attribute_init(struct cafebabe_code_attribute *a,
	struct cafebabe_stream *s);
void cafebabe_code_attribute_deinit(struct cafebabe_code_attribute *a);

#endif
