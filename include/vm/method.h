#ifndef __VM_METHOD_H
#define __VM_METHOD_H

#include "vm/vm.h"
#include "vm/types.h"

#include <stdbool.h>
#include <string.h>

#include "cafebabe/line_number_table_attribute.h"
#include "cafebabe/stack_map_table_attribute.h"
#include "cafebabe/exceptions_attribute.h"
#include "cafebabe/code_attribute.h"
#include "cafebabe/method_info.h"
#include "cafebabe/class.h"

#include "jit/compilation-unit.h"
#include "jit/compiler.h"

#include "lib/buffer.h"

struct vm_class;

#ifdef CONFIG_ARGS_MAP
struct vm_args_map {
	int			reg;
	int			stack_index;
	enum vm_type		type;
};
#endif

struct vm_method_arg {
	struct vm_type_info type_info;
	struct list_head list_node;
};

struct vm_method {
	struct vm_class *class;
	unsigned int method_index;
	unsigned int virtual_index;
	unsigned int itable_index;
	const struct cafebabe_method_info *method;

	char *name;
	char *type;
	int args_count;
#ifdef CONFIG_ARGS_MAP
	struct vm_args_map *args_map;
	int reg_args_count;
#endif
	struct list_head args;
	struct vm_type_info return_type;

	struct cafebabe_code_attribute code_attribute;
	struct cafebabe_line_number_table_attribute line_number_table_attribute;
	struct cafebabe_stack_map_table_attribute stack_map_table_attribute;
	struct cafebabe_exceptions_attribute exceptions_attribute;

	struct compilation_unit *compilation_unit;
	struct jit_trampoline *trampoline;

	char flags;

	unsigned int nr_annotations;
	struct vm_annotation **annotations;
	bool annotation_initialized;
};

#define VM_METHOD_FLAG_VM_NATIVE	(1 << 1)
#define VM_METHOD_FLAG_TRACE		(1 << 2)
#define VM_METHOD_FLAG_TRACE_GATE	(1 << 3)

unsigned int vm_method_arg_stack_count(struct vm_method *vmm);

struct vm_method *vm_method_get_overridden(struct vm_method *vmm);

int vm_method_do_init(struct vm_method *vmm);

int vm_method_init(struct vm_method *vmm,
	struct vm_class *vmc, unsigned int method_index);

int vm_method_init_from_interface(struct vm_method *vmm, struct vm_class *vmc,
	unsigned int method_index, struct vm_method *interface_method);

int vm_method_init_annotation(struct vm_method *vmm);

static inline bool vm_method_is_public(struct vm_method *vmm)
{
	return vmm->method->access_flags & CAFEBABE_METHOD_ACC_PUBLIC;
}

static inline bool vm_method_is_private(struct vm_method *vmm)
{
	return vmm->method->access_flags & CAFEBABE_METHOD_ACC_PRIVATE;
}

static inline bool vm_method_is_static(struct vm_method *vmm)
{
	return vmm->method->access_flags & CAFEBABE_METHOD_ACC_STATIC;
}

static inline bool vm_method_is_final(struct vm_method *vmm)
{
	return vmm->method->access_flags & CAFEBABE_METHOD_ACC_FINAL;
}

static inline bool vm_method_is_native(struct vm_method *vmm)
{
	return vmm->method->access_flags & CAFEBABE_METHOD_ACC_NATIVE;
}

static inline bool vm_method_is_abstract(struct vm_method *vmm)
{
	return vmm->method->access_flags & CAFEBABE_METHOD_ACC_ABSTRACT;
}

static inline bool method_is_synchronized(struct vm_method *vmm)
{
	return vmm->method->access_flags & CAFEBABE_METHOD_ACC_SYNCHRONIZED;
}

static inline bool method_is_private(struct vm_method *vmm)
{
	return vmm->method->access_flags & CAFEBABE_METHOD_ACC_PRIVATE;
}

static inline bool vm_method_is_constructor(struct vm_method *vmm)
{
	return strcmp(vmm->name, "<init>") == 0;
}

static inline bool method_is_virtual(struct vm_method *vmm)
{
	if (vm_method_is_constructor(vmm))
		return false;

	return (vmm->method->access_flags & (CAFEBABE_METHOD_ACC_STATIC
		| CAFEBABE_METHOD_ACC_PRIVATE)) == 0;
}

static inline bool vm_method_is_jni(struct vm_method *vmm)
{
	return vmm->method->access_flags & CAFEBABE_METHOD_ACC_NATIVE
		&& !(vmm->flags & VM_METHOD_FLAG_VM_NATIVE);
}

static inline bool vm_method_is_vm_native(struct vm_method *vmm)
{
	return vmm->method->access_flags & CAFEBABE_METHOD_ACC_NATIVE
		&& (vmm->flags & VM_METHOD_FLAG_VM_NATIVE);
}

static inline bool vm_method_is_traceable(struct vm_method *vmm)
{
	return vmm->flags & VM_METHOD_FLAG_TRACE;
}

static inline bool vm_method_is_trace_gate(struct vm_method *vmm)
{
	return vmm->flags & VM_METHOD_FLAG_TRACE_GATE;
}

static inline bool vm_method_is_special(struct vm_method *vmm)
{
	return vmm->name[0] == '<';
}

static inline bool vm_method_is_compiled(struct vm_method *vmm)
{
	return compilation_unit_is_compiled(vmm->compilation_unit);
}

static inline enum vm_type method_return_type(struct vm_method *method)
{
	char *return_type = index(method->type, ')') + 1;
	return str_to_type(return_type);
}

int vm_method_prepare_jit(struct vm_method *vmm);

static inline void *vm_method_entry_point(struct vm_method *vmm)
{
	return cu_entry_point(vmm->compilation_unit);
}
static inline void *vm_method_ic_entry_point(struct vm_method *vmm)
{
	return cu_ic_entry_point(vmm->compilation_unit);
}

static inline void *vm_method_trampoline_ptr(struct vm_method *vmm)
{
	return buffer_ptr(vmm->trampoline->objcode);
}

static inline void *vm_method_call_ptr(struct vm_method *vmm)
{
	void *result;

	if (vm_method_is_compiled(vmm))
		result = vm_method_entry_point(vmm);
	else
		result = vm_method_trampoline_ptr(vmm);

	return result;
}

static inline bool vm_method_is_missing(struct vm_method *vmm)
{
	return !vmm->compilation_unit;
}

#endif
