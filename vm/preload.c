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
	const char *name;
	struct vm_class **class;
	int optional;
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
	{"boolean", &vm_boolean_class},
	{"char", &vm_char_class},
	{"float", &vm_float_class},
	{"double", &vm_double_class},
	{"byte", &vm_byte_class},
	{"short", &vm_short_class},
	{"int", &vm_int_class},
	{"long", &vm_long_class},
	{"void", &vm_void_class},
};

static const struct preload_entry primitive_array_preload_entries[] = {
	{"[Z", &vm_array_of_boolean},
	{"[C", &vm_array_of_char},
	{"[F", &vm_array_of_float},
	{"[D", &vm_array_of_double},
	{"[B", &vm_array_of_byte},
	{"[S", &vm_array_of_short},
	{"[I", &vm_array_of_int},
	{"[J", &vm_array_of_long},
};

struct field_preload_entry {
	struct vm_class **class;
	const char *name;
	const char *type;
	struct vm_field **field;
	int optional;
};

struct vm_field *vm_java_lang_Class_vmdata;
struct vm_field *vm_java_lang_String_offset;
struct vm_field *vm_java_lang_String_count;
struct vm_field *vm_java_lang_String_value;
struct vm_field *vm_java_lang_Throwable_detailMessage;
struct vm_field *vm_java_lang_VMThrowable_vmdata;
struct vm_field *vm_java_lang_Thread_daemon;
struct vm_field *vm_java_lang_Thread_group;
struct vm_field *vm_java_lang_Thread_name;
struct vm_field *vm_java_lang_Thread_priority;
struct vm_field *vm_java_lang_Thread_contextClassLoader;
struct vm_field *vm_java_lang_Thread_contextClassLoaderIsSystemClassLoader;
struct vm_field *vm_java_lang_Thread_vmThread;
struct vm_field *vm_java_lang_VMThread_thread;
struct vm_field *vm_java_lang_VMThread_vmdata;
struct vm_field *vm_java_lang_reflect_Constructor_clazz;
struct vm_field *vm_java_lang_reflect_Constructor_cons;
struct vm_field *vm_java_lang_reflect_Constructor_slot;
struct vm_field *vm_java_lang_reflect_Field_declaringClass;
struct vm_field *vm_java_lang_reflect_Field_f;
struct vm_field *vm_java_lang_reflect_Field_name;
struct vm_field *vm_java_lang_reflect_Field_slot;
struct vm_field *vm_java_lang_reflect_Method_declaringClass;
struct vm_field *vm_java_lang_reflect_Method_m;
struct vm_field *vm_java_lang_reflect_Method_name;
struct vm_field *vm_java_lang_reflect_Method_slot;
struct vm_field *vm_java_lang_reflect_VMConstructor_clazz;
struct vm_field *vm_java_lang_reflect_VMConstructor_slot;
struct vm_field *vm_java_lang_reflect_VMField_clazz;
struct vm_field *vm_java_lang_reflect_VMField_name;
struct vm_field *vm_java_lang_reflect_VMField_slot;
struct vm_field *vm_java_lang_reflect_VMMethod_clazz;
struct vm_field *vm_java_lang_reflect_VMMethod_name;
struct vm_field *vm_java_lang_reflect_VMMethod_slot;
struct vm_field *vm_java_lang_reflect_VMMethod_m;
struct vm_field *vm_java_lang_ref_Reference_referent;
struct vm_field *vm_java_lang_ref_Reference_lock;
struct vm_field *vm_java_nio_Buffer_address;
struct vm_field *vm_gnu_classpath_PointerNN_data;

static const struct field_preload_entry field_preload_entries[] = {
	{ &vm_java_lang_Class, "vmdata", "Ljava/lang/Object;", &vm_java_lang_Class_vmdata },
	{ &vm_java_lang_String, "offset", "I",	&vm_java_lang_String_offset },
	{ &vm_java_lang_String, "count", "I",	&vm_java_lang_String_count },
	{ &vm_java_lang_String, "value", "[C",	&vm_java_lang_String_value },
	{ &vm_java_lang_Throwable, "detailMessage", "Ljava/lang/String;", &vm_java_lang_Throwable_detailMessage },
	{ &vm_java_lang_VMThrowable, "vmdata", "Ljava/lang/Object;", &vm_java_lang_VMThrowable_vmdata },
	{ &vm_java_lang_Thread, "daemon", "Z", &vm_java_lang_Thread_daemon },
	{ &vm_java_lang_Thread, "group", "Ljava/lang/ThreadGroup;", &vm_java_lang_Thread_group },
	{ &vm_java_lang_Thread, "name", "Ljava/lang/String;", &vm_java_lang_Thread_name },
	{ &vm_java_lang_Thread, "priority", "I", &vm_java_lang_Thread_priority },
	{ &vm_java_lang_Thread, "contextClassLoader", "Ljava/lang/ClassLoader;", &vm_java_lang_Thread_contextClassLoader },
	{ &vm_java_lang_Thread, "contextClassLoaderIsSystemClassLoader", "Z", &vm_java_lang_Thread_contextClassLoaderIsSystemClassLoader },
	{ &vm_java_lang_Thread, "vmThread", "Ljava/lang/VMThread;", &vm_java_lang_Thread_vmThread },
	{ &vm_java_lang_VMThread, "thread", "Ljava/lang/Thread;", &vm_java_lang_VMThread_thread },
	{ &vm_java_lang_VMThread, "vmdata", "Ljava/lang/Object;", &vm_java_lang_VMThread_vmdata },

	/*
	 * java.lang.reflect.Constructor
	 */
	{ &vm_java_lang_reflect_Constructor, "clazz", "Ljava/lang/Class;", &vm_java_lang_reflect_Constructor_clazz, PRELOAD_OPTIONAL },
	{ &vm_java_lang_reflect_Constructor, "slot", "I", &vm_java_lang_reflect_Constructor_slot, PRELOAD_OPTIONAL },
	/* Classpath 0.98 */
	{ &vm_java_lang_reflect_Constructor, "cons", "Ljava/lang/reflect/VMConstructor;", &vm_java_lang_reflect_Constructor_cons, PRELOAD_OPTIONAL },
	{ &vm_java_lang_reflect_VMConstructor, "clazz", "Ljava/lang/Class;", &vm_java_lang_reflect_VMConstructor_clazz, PRELOAD_OPTIONAL },
	{ &vm_java_lang_reflect_VMConstructor, "slot", "I", &vm_java_lang_reflect_VMConstructor_slot, PRELOAD_OPTIONAL },

	/*
	 * java.lang.reflect.Field
	 */
	{ &vm_java_lang_reflect_Field, "declaringClass", "Ljava/lang/Class;", &vm_java_lang_reflect_Field_declaringClass, PRELOAD_OPTIONAL },
	{ &vm_java_lang_reflect_Field, "slot", "I", &vm_java_lang_reflect_Field_slot, PRELOAD_OPTIONAL },
	{ &vm_java_lang_reflect_Field, "name", "Ljava/lang/String;", &vm_java_lang_reflect_Field_name, PRELOAD_OPTIONAL },
	/* Classpath 0.98 */
	{ &vm_java_lang_reflect_Field, "f", "Ljava/lang/reflect/VMField;", &vm_java_lang_reflect_Field_f, PRELOAD_OPTIONAL },
	{ &vm_java_lang_reflect_VMField, "clazz", "Ljava/lang/Class;", &vm_java_lang_reflect_VMField_clazz, PRELOAD_OPTIONAL  },
	{ &vm_java_lang_reflect_VMField, "slot", "I", &vm_java_lang_reflect_VMField_slot, PRELOAD_OPTIONAL },
	{ &vm_java_lang_reflect_VMField, "name", "Ljava/lang/String;", &vm_java_lang_reflect_VMField_name, PRELOAD_OPTIONAL },

	/*
	 * java.lang.reflect.Method
	 */
	{ &vm_java_lang_reflect_Method, "declaringClass", "Ljava/lang/Class;", &vm_java_lang_reflect_Method_declaringClass, PRELOAD_OPTIONAL },
	{ &vm_java_lang_reflect_Method, "name", "Ljava/lang/String;", &vm_java_lang_reflect_Method_name, PRELOAD_OPTIONAL },
	{ &vm_java_lang_reflect_Method, "slot", "I", &vm_java_lang_reflect_Method_slot, PRELOAD_OPTIONAL },
	/* Classpath 0.98 */
	{ &vm_java_lang_reflect_Method, "m", "Ljava/lang/reflect/VMMethod;", &vm_java_lang_reflect_Method_m, PRELOAD_OPTIONAL },
	{ &vm_java_lang_reflect_VMMethod, "clazz", "Ljava/lang/Class;", &vm_java_lang_reflect_VMMethod_clazz, PRELOAD_OPTIONAL },
	{ &vm_java_lang_reflect_VMMethod, "m", "Ljava/lang/reflect/Method;", &vm_java_lang_reflect_VMMethod_m, PRELOAD_OPTIONAL },
	{ &vm_java_lang_reflect_VMMethod, "name", "Ljava/lang/String;", &vm_java_lang_reflect_VMMethod_name, PRELOAD_OPTIONAL },
	{ &vm_java_lang_reflect_VMMethod, "slot", "I", &vm_java_lang_reflect_VMMethod_slot, PRELOAD_OPTIONAL },
	{ &vm_java_lang_ref_Reference, "referent", "Ljava/lang/Object;", &vm_java_lang_ref_Reference_referent},
	{ &vm_java_lang_ref_Reference, "lock", "Ljava/lang/Object;", &vm_java_lang_ref_Reference_lock},

	/*
	 * java/nio/Buffer
	 */
	{ &vm_java_nio_Buffer, "address", "Lgnu/classpath/Pointer;", &vm_java_nio_Buffer_address },

	/*
	 * gnu/classpath/Pointer{32,64}
	 */
#ifdef CONFIG_32_BIT
	{ &vm_gnu_classpath_PointerNN, "data", "I", &vm_gnu_classpath_PointerNN_data },
#else
	{ &vm_gnu_classpath_PointerNN, "data", "J", &vm_gnu_classpath_PointerNN_data },
#endif
};

struct method_preload_entry {
	struct vm_class **class;
	const char *name;
	const char *type;
	struct vm_method **method;
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

		compile_lock_set_status(&cu->compile_lock, STATUS_COMPILED_OK);

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
