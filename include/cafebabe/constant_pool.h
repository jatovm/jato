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

#ifndef CAFEBABE__CONSTANT_POOL_H
#define CAFEBABE__CONSTANT_POOL_H

#include <stdint.h>

struct cafebabe_stream;

enum cafebabe_constant_tag {
	CAFEBABE_CONSTANT_TAG_UTF8			= 1,
	CAFEBABE_CONSTANT_TAG_INTEGER			= 3,
	CAFEBABE_CONSTANT_TAG_FLOAT			= 4,
	CAFEBABE_CONSTANT_TAG_LONG			= 5,
	CAFEBABE_CONSTANT_TAG_DOUBLE			= 6,
	CAFEBABE_CONSTANT_TAG_CLASS			= 7,
	CAFEBABE_CONSTANT_TAG_STRING			= 8,
	CAFEBABE_CONSTANT_TAG_FIELD_REF			= 9,
	CAFEBABE_CONSTANT_TAG_METHOD_REF		= 10,
	CAFEBABE_CONSTANT_TAG_INTERFACE_METHOD_REF	= 11,
	CAFEBABE_CONSTANT_TAG_NAME_AND_TYPE		= 12,
};

struct cafebabe_constant_info_utf8 {
	uint16_t length;
	uint8_t *bytes;
};

struct cafebabe_constant_info_integer {
	uint32_t bytes;
};

struct cafebabe_constant_info_float {
	uint32_t bytes;
};

struct cafebabe_constant_info_long {
	uint32_t high_bytes;
	uint32_t low_bytes;
};

struct cafebabe_constant_info_double {
	uint32_t high_bytes;
	uint32_t low_bytes;
};

struct cafebabe_constant_info_class {
	uint16_t name_index;
};

struct cafebabe_constant_info_string {
	uint16_t string_index;
};

struct cafebabe_constant_info_field_ref {
	uint16_t class_index;
	uint16_t name_and_type_index;
};

struct cafebabe_constant_info_method_ref {
	uint16_t class_index;
	uint16_t name_and_type_index;
};

struct cafebabe_constant_info_interface_method_ref {
	uint16_t class_index;
	uint16_t name_and_type_index;
};

struct cafebabe_constant_info_name_and_type {
	uint16_t name_index;
	uint16_t descriptor_index;
};

struct cafebabe_constant_pool {
	enum cafebabe_constant_tag tag;
	union {
		struct cafebabe_constant_info_utf8 utf8;
		struct cafebabe_constant_info_integer integer_;
		struct cafebabe_constant_info_float float_;
		struct cafebabe_constant_info_long long_;
		struct cafebabe_constant_info_double double_;
		struct cafebabe_constant_info_class class;
		struct cafebabe_constant_info_string string;
		struct cafebabe_constant_info_field_ref field_ref;
		struct cafebabe_constant_info_method_ref method_ref;
		struct cafebabe_constant_info_interface_method_ref interface_method_ref;
		struct cafebabe_constant_info_name_and_type name_and_type;
	};
};

int cafebabe_constant_pool_init(struct cafebabe_constant_pool *cp,
	struct cafebabe_stream *s);
void cafebabe_constant_pool_deinit(struct cafebabe_constant_pool *cp);

#endif
