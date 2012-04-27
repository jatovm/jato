#include "arch/inline-cache.h"

#include "jit/bc-offset-mapping.h"
#include "jit/compilation-unit.h"
#include "jit/inline-cache.h"
#include "jit/instruction.h"
#include "jit/cu-mapping.h"

#include "vm/method.h"
#include "vm/class.h"
#include "vm/trace.h"
#include "vm/die.h"

#include "arch/instruction.h"
#include "arch/isa.h"

#include <stdbool.h>
#include <pthread.h>
#include <assert.h>

#define X86_MOV_IMM_REG_INSN_SIZE 	5
#define X86_MOV_IMM_REG_IMM_OFFSET 	1
#define X86_MOV_EAX_OPC 		0xb8

struct x86_ic {
	unsigned long		fn;
	unsigned long		imm;
};

static pthread_mutex_t ic_patch_lock = PTHREAD_MUTEX_INITIALIZER;


static void ic_from_callsite(struct x86_ic *ic, unsigned long callsite)
{
	ic->fn	= callsite + X86_CALL_DISP_OFFSET;

	ic->imm	= callsite - X86_MOV_IMM_REG_INSN_SIZE + X86_MOV_IMM_REG_IMM_OFFSET;
}

static inline unsigned long x86_call_disp(void *callsite, void *target)
{
	return (unsigned long) target - (unsigned long) callsite - X86_CALL_INSN_SIZE;
}

static inline bool is_valid_ic(struct x86_ic *ic)
{
	if (*(((unsigned char *)ic->fn) - X86_CALL_DISP_OFFSET) != X86_CALL_OPC)
		return false;
	if (*(((unsigned char *)ic->imm) - X86_MOV_IMM_REG_IMM_OFFSET) != X86_MOV_EAX_OPC)
		return false;
	return true;
}

static void patch_this_operand(struct insn *class_insn, struct insn *ic_call_insn)
{
	/*
	 * The struct use_position for ic_call_insn->src will be correctly
	 * filled in to point to the correct register after liveness analysis
	 * and register allocation. The operand ic_call_insn->src is simply a
	 * placeholder for class_insn->src.base_reg. We have to copy the
	 * struct use_position and patch it to maintain its state correctly.
	 */
	class_insn->src.type = OPERAND_MEMBASE;
	class_insn->src.disp = offsetof(struct vm_object, class);
	class_insn->src.base_reg = ic_call_insn->src.reg;
	class_insn->src.base_reg.insn = class_insn;
	list_add_tail(&class_insn->src.base_reg.use_pos_list,
		      &ic_call_insn->src.reg.use_pos_list);

}

void *ic_lookup_vtable(struct vm_class *vmc, struct vm_method *vmm)
{
	assert(vmc);
	assert(vmm);

	assert(vmm->virtual_index < vmc->vtable_size);

	return vmc->vtable.native_ptr[vmm->virtual_index];
}

bool ic_supports_method(struct vm_method *vmm)
{
	assert(vmm != NULL);

	return method_is_virtual(vmm)
		&& !vm_method_is_native(vmm)
		&& !vm_method_is_special(vmm)
		&& vmm->class
		&& !vm_class_is_primitive_class(vmm->class);
}

static void ic_set_to_monomorphic(struct vm_class *vmc, struct vm_method *vmm, void *callsite)
{
	struct x86_ic ic;
	void *ic_entry_point;

	assert(vmm);
	assert(vmc);
	assert(callsite);
	assert(vm_method_is_compiled(vmm));

	ic_from_callsite(&ic, (unsigned long) callsite);
	ic_entry_point = vm_method_ic_entry_point(vmm);

	assert(is_valid_ic(&ic));
	assert(ic_entry_point);

	if (pthread_mutex_lock(&ic_patch_lock) != 0)
		die("Failed to lock ic_patch_lock\n");

	cpu_write_u32((void *) ic.fn, x86_call_disp(callsite, ic_entry_point));
	cpu_write_u32((void *) ic.imm, (unsigned long) vmc);

	if (pthread_mutex_unlock(&ic_patch_lock) != 0)
		die("Failed to unlock ic_patch_lock\n");
}

static void ic_set_to_megamorphic(struct vm_method *vmm, void *callsite)
{
	struct x86_ic ic;

	assert(vmm);
	assert(callsite);

	ic_from_callsite(&ic, (unsigned long)callsite);
	assert(is_valid_ic(&ic));

	if (pthread_mutex_lock(&ic_patch_lock) != 0)
		die("Failed to lock ic_patch_lock\n");
	cpu_write_u32((void *) ic.fn, x86_call_disp(callsite, ic_vcall_stub));
	cpu_write_u32((void *) ic.imm, (uint32_t)(vmm->virtual_index * sizeof(void *)));
	if (pthread_mutex_unlock(&ic_patch_lock) != 0)
		die("Failed to unlock ic_patch_lock\n");
}

void *do_ic_setup(struct vm_class *vmc, struct vm_method *i_vmm, void *return_addr)
{
	void *callsite = return_addr - X86_CALL_INSN_SIZE;
	struct compilation_unit *cu;
	struct vm_method *c_vmm;
	void *entry_point;
	struct x86_ic ic;

	assert(vmc);
	assert(i_vmm);
	assert(callsite);

	ic_from_callsite(&ic, (unsigned long) callsite);
	assert(is_valid_ic(&ic));

	entry_point = ic_lookup_vtable(vmc, i_vmm);
	assert(entry_point);

	cu = jit_lookup_cu((unsigned long)entry_point);
	assert(cu);

	c_vmm = cu->method;

	assert(c_vmm);

	if (vm_method_is_compiled(c_vmm)) {
		ic_set_to_monomorphic(vmc, c_vmm, callsite);
	}

	return vm_method_call_ptr(c_vmm);
}

int convert_ic_calls(struct compilation_unit *cu)
{
	struct ic_call *ic, *tmp;
	struct insn *insn;
	struct insn *class_insn, *imm_insn, *call_insn;
	struct list_head *ic_call;
	unsigned long bc_offset;
	struct var_info *imm_reg, *class_reg;

	imm_reg = class_reg = NULL;

	if (!list_is_empty(&cu->ic_call_list)) {
		imm_reg = get_fixed_var(cu, IC_IMM_REG);
		class_reg = get_fixed_var(cu, IC_CLASS_REG);
	}

	list_for_each_entry_safe(ic, tmp, &cu->ic_call_list, list_node) {

		insn = ic->insn;
		ic_call = &insn->insn_list_node;

		class_insn = 	membase_reg_insn(INSN_MOV_MEMBASE_REG,
					      insn->src.reg.interval->var_info,
					      offsetof(struct vm_object, class),
					      class_reg);

		if (!class_insn)
			return -ENOMEM;

		patch_this_operand(class_insn, insn);

		imm_insn = 	imm_reg_insn(INSN_MOV_IMM_REG,
					insn->dest.imm,
					imm_reg);

		if (!imm_insn)
			return -ENOMEM;

		call_insn = rel_insn(INSN_CALL_REL, (unsigned long)ic_start);

		if (!call_insn)
			return -ENOMEM;

		bc_offset = insn_get_bc_offset(insn);
		insn_set_bc_offset(class_insn, bc_offset);
		insn_set_bc_offset(imm_insn, bc_offset);
		insn_set_bc_offset(call_insn, bc_offset);


		list_add_tail(&class_insn->insn_list_node, ic_call);
		list_add_tail(&imm_insn->insn_list_node, ic_call);
		list_add_tail(&call_insn->insn_list_node, ic_call);

		list_del(ic_call);
		free_insn(insn);
	}

	return 0;
}

void *resolve_ic_miss(struct vm_class *vmc, struct vm_method *vmm, void *return_addr)
{
	void *callsite = return_addr - X86_CALL_INSN_SIZE;

	ic_set_to_megamorphic(vmm, callsite);

	return ic_lookup_vtable(vmc, vmm);
}
