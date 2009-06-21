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

