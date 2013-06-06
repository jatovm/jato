/*
 * Copyright (c) 2009  Vegard Nossum
 *               2009  Tomasz Grabiec
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

#include <stdio.h>

#include "vm/die.h"
#include "vm/classloader.h"
#include "vm/natives.h"
#include "vm/preload.h"
#include "vm/class.h"

#include "jit/cu-mapping.h"

enum {
	PRELOAD_MANDATORY	= 0x00,
	PRELOAD_OPTIONAL	= 0x01,
};

struct preload_entry {
	const char		*name;
	struct vm_class		**class;
	int			optional;
};

#define PRELOAD_CLASS(class_name, var_name, opt) \
	struct vm_class *var_name;
#include "vm/preload-classes.h"
#undef PRELOAD_CLASS

struct vm_class *vm_boolean_class;
struct vm_class *vm_char_class;
struct vm_class *vm_float_class;
struct vm_class *vm_double_class;
struct vm_class *vm_byte_class;
struct vm_class *vm_short_class;
struct vm_class *vm_int_class;
struct vm_class *vm_long_class;
struct vm_class *vm_void_class;
struct vm_class *vm_array_of_boolean;
struct vm_class *vm_array_of_byte;
struct vm_class *vm_array_of_char;
struct vm_class *vm_array_of_double;
struct vm_class *vm_array_of_float;
struct vm_class *vm_array_of_int;
struct vm_class *vm_array_of_long;
struct vm_class *vm_array_of_short;

static const struct preload_entry preload_entries[] = {
#define PRELOAD_CLASS(class_name, var_name, opt) \
	{ class_name, &var_name, opt },
#include "vm/preload-classes.h"
#undef PRELOAD_CLASS
};

static const struct preload_entry primitive_preload_entries[] = {
	{"boolean",	&vm_boolean_class,	PRELOAD_MANDATORY},
	{"char",	&vm_char_class,		PRELOAD_MANDATORY},
	{"float",	&vm_float_class,	PRELOAD_MANDATORY},
	{"double",	&vm_double_class,	PRELOAD_MANDATORY},
	{"byte",	&vm_byte_class,		PRELOAD_MANDATORY},
	{"short",	&vm_short_class,	PRELOAD_MANDATORY},
	{"int",		&vm_int_class,		PRELOAD_MANDATORY},
	{"long",	&vm_long_class,		PRELOAD_MANDATORY},
	{"void",	&vm_void_class,		PRELOAD_MANDATORY},
};

static const struct preload_entry primitive_array_preload_entries[] = {
	{"[Z",		&vm_array_of_boolean,	PRELOAD_MANDATORY},
	{"[C",		&vm_array_of_char,	PRELOAD_MANDATORY},
	{"[F",		&vm_array_of_float,	PRELOAD_MANDATORY},
	{"[D",		&vm_array_of_double,	PRELOAD_MANDATORY},
	{"[B",		&vm_array_of_byte,	PRELOAD_MANDATORY},
	{"[S",		&vm_array_of_short,	PRELOAD_MANDATORY},
	{"[I",		&vm_array_of_int,	PRELOAD_MANDATORY},
	{"[J",		&vm_array_of_long,	PRELOAD_MANDATORY},
};

struct field_preload_entry {
	struct vm_class		**class;
	const char		*name;
	const char		*type;
	struct vm_field		**field;
	int			optional;
};

#define PRELOAD_FIELD(c, n, t, o)			\
	struct vm_field *c##_##n;
#include "vm/preload-fields.h"
#undef PRELOAD_FIELD

#define STRINGIFY(x) #x

#define PRELOAD_FIELD(c, n, t, o)			\
	{						\
		.class		= &c,			\
		.name		= STRINGIFY(n),		\
		.type		= t,			\
		.field		= &c##_##n,		\
		.optional	= o			\
	},

static const struct field_preload_entry field_preload_entries[] = {
#include "vm/preload-fields.h"
};

#undef PRELOAD_FIELD

struct method_preload_entry {
	struct vm_class		**class;
	const char		*name;
	const char		*type;
	struct vm_method	**method;
};

#define PRELOAD_METHOD(class, method_name, method_type, var_name) \
	struct vm_method *var_name;
#include "vm/preload-methods.h"
#undef PRELOAD_METHOD

static const struct method_preload_entry method_preload_entries[] = {
#define PRELOAD_METHOD(class, method_name, method_type, var_name) \
	{ &class, method_name, method_type, &var_name },
#include "vm/preload-methods.h"
#undef PRELOAD_METHOD
};

/*
 * Methods put in this table will be forcibly marked as native which
 * will allow VM to provide its own impementation for them.
 */
static struct vm_method **native_override_entries[] = {
	&vm_java_lang_VMString_intern,
};

bool preload_finished;

int nr_class_fixups;
struct vm_class **class_fixups;

int vm_preload_add_class_fixup(struct vm_class *vmc)
{
	int new_size = sizeof(struct vm_class *) * (nr_class_fixups + 1);
	struct vm_class **new_array = realloc(class_fixups, new_size);

	if (!new_array)
		return -ENOMEM;

	class_fixups = new_array;
	class_fixups[nr_class_fixups++] = vmc;
	return 0;
}

int preload_vm_classes(void)
{
	for (unsigned int i = 0; i < ARRAY_SIZE(preload_entries); ++i) {
		const struct preload_entry *pe = &preload_entries[i];

		struct vm_class *class = classloader_load(NULL, pe->name);
		if (!class) {
			if (pe->optional == PRELOAD_OPTIONAL)
				continue;
			warn("preload of %s failed", pe->name);
			return -EINVAL;
		}

		*pe->class = class;
	}

	for (unsigned int i = 0; i < ARRAY_SIZE(primitive_preload_entries); ++i) {
		const struct preload_entry *pe = &primitive_preload_entries[i];

		struct vm_class *class = classloader_load_primitive(pe->name);
		if (!class) {
			if (pe->optional == PRELOAD_OPTIONAL)
				continue;
			warn("preload of %s failed", pe->name);
			return -EINVAL;
		}

		*pe->class = class;
	}

	for (unsigned int i = 0; i < ARRAY_SIZE(primitive_array_preload_entries); ++i) {
		const struct preload_entry *pe = &primitive_array_preload_entries[i];

		struct vm_class *class = classloader_load(NULL, pe->name);
		if (!class) {
			if (pe->optional == PRELOAD_OPTIONAL)
				continue;
			warn("preload of %s failed", pe->name);
			return -EINVAL;
		}

		*pe->class = class;
	}

	for (unsigned int i = 0; i < ARRAY_SIZE(field_preload_entries); ++i) {
		const struct field_preload_entry *pe = &field_preload_entries[i];

		if (!*pe->class && pe->optional == PRELOAD_OPTIONAL)
			continue;

		struct vm_field *field = vm_class_get_field(*pe->class, pe->name, pe->type);
		if (!field) {
			if (pe->optional == PRELOAD_OPTIONAL)
				continue;
			warn("preload of %s.%s %s failed", (*pe->class)->name, pe->name, pe->type);
			return -EINVAL;
		}

		*pe->field = field;
	}

	for (unsigned int i = 0; i < ARRAY_SIZE(method_preload_entries); ++i) {
		const struct method_preload_entry *me
			= &method_preload_entries[i];

		struct vm_method *method = vm_class_get_method(*me->class,
			me->name, me->type);
		if (!method) {
			warn("preload of %s.%s%s failed", (*me->class)->name,
			     me->name, me->type);
			return -EINVAL;
		}

		*me->method = method;
	}

	for (unsigned int i = 0; i < ARRAY_SIZE(native_override_entries); ++i) {
		struct cafebabe_method_info *m_info;
		struct compilation_unit *cu;
		struct vm_method *vmm;

		vmm = *native_override_entries[i];
		vmm->flags |= VM_METHOD_FLAG_VM_NATIVE;

		cu = vmm->compilation_unit;

		cu->entry_point = vm_lookup_native(vmm->class->name, vmm->name);
		if (!cu->entry_point)
			error("no VM native for overriden method: %s.%s%s",
			      vmm->class->name, vmm->name, vmm->type);

		cu->state = COMPILATION_STATE_COMPILED;

		if (add_cu_mapping((unsigned long)cu->entry_point, cu))
			return -EINVAL;

		m_info = (struct cafebabe_method_info *)vmm->method;
		m_info->access_flags |= CAFEBABE_METHOD_ACC_NATIVE;
	}

	preload_finished = true;

	for (int i = 0; i < nr_class_fixups; ++i) {
		struct vm_class *vmc = class_fixups[i];

		if (vm_class_setup_object(vmc)) {
			warn("fixup failed for %s", vmc->name);
			return -EINVAL;
		}
	}

	free(class_fixups);

	return 0;
}
