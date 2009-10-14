/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2008  Vegard Nossum <vegardno@ifi.uio.no>
 *
 * This file is released under the GPL version 2 with the following
 * clarification and special exception:
 *
 *     Linking this library statically or dynamically with other modules is
 *     making a combined work based on this library. Thus, the terms and
 *     conditions of the GNU General Public License cover the whole
 *     combination.
 *
 *     As a special exception, the copyright holders of this library give you
 *     permission to link this library with independent modules to produce an
 *     executable, regardless of the license terms of these independent
 *     modules, and to copy and distribute the resulting executable under terms
 *     of your choice, provided that you also meet, for each linked independent
 *     module, the terms and conditions of the license of that module. An
 *     independent module is a module which is not derived from or based on
 *     this library. If you modify this library, you may extend this exception
 *     to your version of the library, but you are not obligated to do so. If
 *     you do not wish to do so, delete this exception statement from your
 *     version.
 *
 * Please refer to the file LICENSE for details.
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

/**
 * An entry in the constant pool table.
 *
 * See also section 4.4 of The Java Virtual Machine Specification.
 */
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

int cafebabe_constant_info_utf8_compare(
	const struct cafebabe_constant_info_utf8 *s1, const char *s2);

#endif
