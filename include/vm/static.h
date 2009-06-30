#ifndef _VM_STATIC_H
#define _VM_STATIC_H

extern void *static_guard_page;

void static_fixup_init(void);

struct insn;
struct vm_class;

enum static_fixup_type {
	STATIC_FIXUP_GETSTATIC,
	STATIC_FIXUP_PUTSTATIC,
};

struct static_fixup_site {
	struct list_head vmc_node;
	struct list_head cu_node;

	enum static_fixup_type type;
	struct insn *insn;
	struct vm_field *vmf;
	struct compilation_unit *cu;
};

int add_getstatic_fixup_site(struct insn *insn,
	struct vm_field *vmf, struct compilation_unit *cu);
int add_putstatic_fixup_site(struct insn *insn,
	struct vm_field *vmf, struct compilation_unit *cu);

void fixup_static(struct vm_class *vmc);
void fixup_static_at(unsigned long addr);

extern unsigned long static_field_signal_bh(unsigned long ret);

#endif
