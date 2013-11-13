/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2010  Pekka Enberg
 *
 * This file is released under the 2-clause BSD license. Please refer to the
 * file LICENSE for details.
 */

#ifndef CAFEBABE__ANNOTATION_H
#define CAFEBABE__ANNOTATION_H

#include <stdint.h>

#include "cafebabe/attribute_array.h"
#include "cafebabe/attribute_info.h"

#define ELEMENT_TYPE_BYTE			'B'
#define ELEMENT_TYPE_CHAR			'C'
#define ELEMENT_TYPE_DOUBLE			'D'
#define ELEMENT_TYPE_FLOAT			'F'
#define ELEMENT_TYPE_INTEGER			'I'
#define ELEMENT_TYPE_LONG			'J'
#define ELEMENT_TYPE_SHORT			'S'
#define ELEMENT_TYPE_BOOLEAN			'Z'
#define ELEMENT_TYPE_STRING			's'
#define ELEMENT_TYPE_ENUM_CONSTANT		'e'
#define ELEMENT_TYPE_CLASS			'c'
#define ELEMENT_TYPE_ANNOTATION_TYPE		'@'
#define ELEMENT_TYPE_ARRAY			'['

struct cafebabe_annotation;

struct cafebabe_element_value {
	uint8_t					tag;			/* See ELEMENT_TYPE_<type> above. */
	union {
		uint16_t			const_value_index;
		struct {
			uint16_t		type_name_index;
			uint16_t		const_name_index;
		} enum_const_value;
		uint16_t			class_info_index;
		struct cafebabe_annotation	*annotation_value;
		struct {
			uint16_t			num_values;
			struct cafebabe_element_value	*values;
		} array_value;
	} value;
};

struct cafebabe_element_value_pair {
	uint16_t				element_name_index;
	struct cafebabe_element_value		value;
};

struct cafebabe_annotation {
	uint16_t				type_index;
	uint16_t				num_element_value_pairs;
	struct cafebabe_element_value_pair	*element_value_pairs;
};

#endif
