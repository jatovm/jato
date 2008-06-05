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

#ifndef CAFEBABE__METHOD_INFO_H
#define CAFEBABE__METHOD_INFO_H

#include <stdint.h>

struct cafebabe_attribute_info;
struct cafebabe_stream;

/**
 * A java class method.
 *
 * See also section 4.6 of The Java Virtual Machine Specification.
 */
struct cafebabe_method_info {
	uint16_t access_flags;
	uint16_t name_index;
	uint16_t descriptor_index;
	uint16_t attributes_count;
	struct cafebabe_attribute_info *attributes;
};

int cafebabe_method_info_init(struct cafebabe_method_info *m,
	struct cafebabe_stream *s);
void cafebabe_method_info_deinit(struct cafebabe_method_info *m);

#endif
