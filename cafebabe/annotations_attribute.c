/*
 * cafebabe - the class loader library in C
 * Copyright (C) 2010  Pekka Enberg
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

#include "cafebabe/annotations_attribute.h"

#include <stdlib.h>
#include <string.h>

#include "cafebabe/attribute_info.h"
#include "cafebabe/stream.h"
#include "cafebabe/class.h"

#include "vm/die.h"

static int cafebabe_annotation_parse(struct cafebabe_annotation *a, struct cafebabe_stream *s);

static int
cafebabe_element_value_parse(struct cafebabe_element_value *v, struct cafebabe_stream *s)
{
	int err;

	err = cafebabe_stream_read_uint8(s, &v->tag);
	if (err)
		goto out;

	switch (v->tag) {
	case ELEMENT_TYPE_BYTE:
	case ELEMENT_TYPE_CHAR:
	case ELEMENT_TYPE_DOUBLE:
	case ELEMENT_TYPE_FLOAT:
	case ELEMENT_TYPE_INTEGER:
	case ELEMENT_TYPE_LONG:
	case ELEMENT_TYPE_SHORT:
	case ELEMENT_TYPE_BOOLEAN:
	case ELEMENT_TYPE_STRING: {
		err = cafebabe_stream_read_uint16(s, &v->value.const_value_index);
		if (err)
			goto out;
		break;
	}
	case ELEMENT_TYPE_ENUM_CONSTANT: {
		err = cafebabe_stream_read_uint16(s, &v->value.enum_const_value.type_name_index);
		if (err)
			goto out;

		err = cafebabe_stream_read_uint16(s, &v->value.enum_const_value.const_name_index);
		if (err)
			goto out;
		break;
	}
	case ELEMENT_TYPE_CLASS: {
		err = cafebabe_stream_read_uint16(s, &v->value.class_info_index);
		if (err)
			goto out;
		break;
	}
	case ELEMENT_TYPE_ANNOTATION_TYPE: {
		v->value.annotation_value = malloc(sizeof(struct cafebabe_annotation));
		if (!v->value.annotation_value)
			goto out;

		err = cafebabe_annotation_parse(v->value.annotation_value, s);
		if (err)
			goto out;
		break;
	}
	case ELEMENT_TYPE_ARRAY: {
		err = cafebabe_stream_read_uint16(s, &v->value.array_value.num_values);
		if (err)
			goto out;

		v->value.array_value.values = calloc(v->value.array_value.num_values, sizeof(struct cafebabe_element_value));
		for (unsigned int i = 0; i < v->value.array_value.num_values; i++) {
			struct cafebabe_element_value *array_value = &v->value.array_value.values[i];

			err = cafebabe_element_value_parse(array_value, s);
			if (err)
				goto out;
		}
		break;
	}
	default:
		warn("unknown annotation element type %d", v->tag);
		err = -1;
		goto out;
	};
out:
	return err;
}

static int
cafebabe_element_value_pair_parse(struct cafebabe_element_value_pair *p, struct cafebabe_stream *s)
{
	int err;

	err = cafebabe_stream_read_uint16(s, &p->element_name_index);
	if (err)
		goto out;

	err = cafebabe_element_value_parse(&p->value, s);
out:
	return err;
}

static int
cafebabe_annotation_parse(struct cafebabe_annotation *a, struct cafebabe_stream *s)
{
	int err;

	err = cafebabe_stream_read_uint16(s, &a->type_index);
	if (err)
		goto out;

	err = cafebabe_stream_read_uint16(s, &a->num_element_value_pairs);
	if (err)
		goto out;

	a->element_value_pairs	= calloc(a->num_element_value_pairs, sizeof(struct cafebabe_element_value_pair));
	if (!a->element_value_pairs) {
		err = -1;
		goto out;
	}
	for (unsigned int i = 0; i < a->num_element_value_pairs; i++) {
		err = cafebabe_element_value_pair_parse(&a->element_value_pairs[i], s);
		if (err)
			goto out;
	}
out:
	return err;
}

int
cafebabe_annotations_attribute_init(struct cafebabe_annotations_attribute *a, struct cafebabe_stream *s)
{
	int err = 0;

	err = cafebabe_stream_read_uint16(s, &a->num_annotations);
	if (err)
		goto out;

	a->annotations	= calloc(a->num_annotations, sizeof(struct cafebabe_annotation));
	if (!a->annotations) {
		err = -1;
		goto out;
	}
	for (unsigned int i = 0; i < a->num_annotations; i++) {
		err = cafebabe_annotation_parse(&a->annotations[i], s);
		if (err)
			goto out;
	}
out:
	return err;
}

static void
cafebabe_annotation_free(struct cafebabe_annotation *a)
{
	for (unsigned int i = 0; i < a->num_element_value_pairs; i++) {
		struct cafebabe_element_value_pair *ev = &a->element_value_pairs[i];

		switch (ev->value.tag) {
		case ELEMENT_TYPE_ANNOTATION_TYPE:
			free(ev->value.value.annotation_value);
			break;
		}
	}
	free(a->element_value_pairs);
}

void
cafebabe_annotations_attribute_deinit(struct cafebabe_annotations_attribute *a)
{
	for (unsigned int i = 0; i < a->num_annotations; i++)
		cafebabe_annotation_free(&a->annotations[i]);

	free(a->annotations);
}

int
cafebabe_read_annotations_attribute(const struct cafebabe_class *class,
				    const struct cafebabe_attribute_array *attributes,
				    struct cafebabe_annotations_attribute *annotations_attrib)
{
	const struct cafebabe_attribute_info *attribute;
	unsigned int annotations_index = 0;
	struct cafebabe_stream stream;
	int err;

	memset(annotations_attrib, 0, sizeof(*annotations_attrib));

	if (cafebabe_attribute_array_get(attributes, "RuntimeVisibleAnnotations", class, &annotations_index))
		return 0;

	attribute = &class->attributes.array[annotations_index];

	cafebabe_stream_open_buffer(&stream, attribute->info, attribute->attribute_length);

	err = cafebabe_annotations_attribute_init(annotations_attrib, &stream);

	cafebabe_stream_close_buffer(&stream);

	return err;
}
