/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#pragma once

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
