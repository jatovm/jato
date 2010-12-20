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

#ifndef CAFEBABE__CLASS_H
#define CAFEBABE__CLASS_H

#include "vm/jni.h"

#include "cafebabe/attribute_array.h"

#include <stdint.h>

struct cafebabe_attribute_info;
struct cafebabe_constant_info_utf8;
struct cafebabe_constant_info_class;
struct cafebabe_constant_info_field_ref;
struct cafebabe_constant_info_method_ref;
struct cafebabe_constant_info_interface_method_ref;
struct cafebabe_constant_info_name_and_type;
struct cafebabe_constant_pool;
struct cafebabe_field_info;
struct cafebabe_method_info;
struct cafebabe_stream;

#define CAFEBABE_CLASS_ACC_PUBLIC	0x0001
#define CAFEBABE_CLASS_ACC_PRIVATE	0x0002
#define CAFEBABE_CLASS_ACC_PROTECTED	0x0004
#define CAFEBABE_CLASS_ACC_STATIC	0x0008
#define CAFEBABE_CLASS_ACC_FINAL	0x0010
#define CAFEBABE_CLASS_ACC_SUPER	0x0020
#define CAFEBABE_CLASS_ACC_INTERFACE	0x0200
#define CAFEBABE_CLASS_ACC_ABSTRACT	0x0400
#define CAFEBABE_CLASS_ACC_ANNOTATION	0x4000

/**
 * A java class file.
 *
 * See also section 4.1 of The Java Virtual Machine Specification.
 */
struct cafebabe_class {
	uint32_t magic;
	uint16_t minor_version;
	uint16_t major_version;
	uint16_t constant_pool_count;
	struct cafebabe_constant_pool *constant_pool;
	uint16_t access_flags;
	uint16_t this_class;
	uint16_t super_class;
	uint16_t interfaces_count;
	uint16_t *interfaces;
	uint16_t fields_count;
	struct cafebabe_field_info *fields;
	uint16_t methods_count;
	struct cafebabe_method_info *methods;
	struct cafebabe_attribute_array attributes;
};

int cafebabe_class_init(struct cafebabe_class *c,
	struct cafebabe_stream *s);
void cafebabe_class_deinit(struct cafebabe_class *c);

int cafebabe_class_constant_index_invalid(const struct cafebabe_class *c,
	uint16_t i);
int cafebabe_class_constant_get_integer(const struct cafebabe_class *c, uint16_t i, jint *value);
int cafebabe_class_constant_get_long(const struct cafebabe_class *c, uint16_t i, jlong *value);
int cafebabe_class_constant_get_float(const struct cafebabe_class *c, uint16_t i, jfloat *value);
int cafebabe_class_constant_get_double(const struct cafebabe_class *c, uint16_t i, jdouble *value);
int cafebabe_class_constant_get_utf8(const struct cafebabe_class *c,
	uint16_t i, const struct cafebabe_constant_info_utf8 **r);
int cafebabe_class_constant_get_class(const struct cafebabe_class *c,
	uint16_t i, const struct cafebabe_constant_info_class **r);
int cafebabe_class_constant_get_field_ref(const struct cafebabe_class *c,
	uint16_t i, const struct cafebabe_constant_info_field_ref **r);
int cafebabe_class_constant_get_method_ref(const struct cafebabe_class *c,
	uint16_t i, const struct cafebabe_constant_info_method_ref **r);
int cafebabe_class_constant_get_interface_method_ref(
	const struct cafebabe_class *c, uint16_t i,
	const struct cafebabe_constant_info_interface_method_ref **r);
int cafebabe_class_constant_get_name_and_type(const struct cafebabe_class *c,
	uint16_t i, const struct cafebabe_constant_info_name_and_type **r);

int cafebabe_class_get_field(const struct cafebabe_class *c,
	const char *name, const char *descriptor,
	unsigned int *r);
int cafebabe_class_get_method(const struct cafebabe_class *c,
	const char *name, const char *descriptor,
	unsigned int *r);
char *cafebabe_class_get_source_file_name(const struct cafebabe_class *class);

#endif
