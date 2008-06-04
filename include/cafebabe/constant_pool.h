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
};

struct cafebabe_constant_info_float {
};

struct cafebabe_constant_info_long {
};

struct cafebabe_constant_info_double {
};

struct cafebabe_constant_info_class {
	uint16_t name_index;
};

struct cafebabe_constant_info_string {
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
};

struct cafebabe_constant_info_name_and_type {
	uint16_t name_index;
	uint16_t descriptor_index;
};

struct cafebabe_constant_pool {
	enum cafebabe_constant_tag tag;
	union {
		struct cafebabe_constant_info_utf8 *utf8;
		struct cafebabe_constant_info_integer *integer;
		struct cafebabe_constant_info_float *float_;
		struct cafebabe_constant_info_long *long_;
		struct cafebabe_constant_info_double *double_;
		struct cafebabe_constant_info_class *class;
		struct cafebabe_constant_info_string *string;
		struct cafebabe_constant_info_field_ref *field_ref;
		struct cafebabe_constant_info_method_ref *method_ref;
		struct cafebabe_constant_info_interface_method_ref *interface_method_ref;
		struct cafebabe_constant_info_name_and_type *name_and_type;
	} info;
};

int cafebabe_constant_pool_init(struct cafebabe_constant_pool *cp,
	struct cafebabe_stream *s);
void cafebabe_constant_pool_deinit(struct cafebabe_constant_pool *cp);

#endif
