#ifndef __VM_VERIFIER_H
#define __VM_VERIFIER_H

#include "vm/types.h"
#include "lib/list.h"

struct verifier_operand {
	enum vm_type			vm_type;
	bool				is_fragment;		/* If it is not the first slot of a multi-slot op */
};

struct verifier_stack {
	struct verifier_operand		op;
	struct list_head		slots;
};

enum verifier_local_var_state {
	DEFINED,
	UNDEFINED,
	UNKNOWN,
};

struct verifier_local_var {
	enum verifier_local_var_state	state;
	struct verifier_operand		op;
};

struct verifier_state {
	struct verifier_local_var	*vars;			/* Array of local_vars. */
	unsigned int			nb_vars;
	struct verifier_stack		*stack;			/* Stack of verifier_operands stored as a list. */
};

struct verifier_block {
	uint32_t 			begin_offset;
	uint32_t 			*following_offsets;
	uint32_t 			nb_followers;

	struct verifier_state		*initial_state;
	struct verifier_state 		*final_state;

	unsigned char			*code;
	unsigned long			pc;
	unsigned char			opc;
	bool				is_wide;

	struct verifier_context		*parent_ctx;
	struct list_head 		blocks;
};

struct verifier_jump_destinations {
	unsigned long			dest;
	struct list_head		list;
};

struct verifier_context {
	struct vm_method			*method;
	unsigned char				*code;
	unsigned long				code_size;

	unsigned long				max_stack;
	unsigned long				max_locals;

	struct verifier_jump_destinations	*jmp_dests;
	struct verifier_block			*vb_list;
	bool					is_wide;
};

struct verifier_local_var *alloc_verifier_local_var(int nb_vars);
struct verifier_stack *alloc_verifier_stack(enum vm_type vm_type);
struct verifier_state *alloc_verifier_state(int nb_vars);
struct verifier_block *alloc_verifier_block(struct verifier_context *vrf, uint32_t begin_offset);
struct verifier_jump_destinations *alloc_verifier_jump_destinations(unsigned long initial_size);
struct verifier_context *alloc_verifier_context(struct vm_method *vmm);

void free_verifier_local_var(struct verifier_local_var *vars);
void free_verifier_stack(struct verifier_stack *stack);
void free_verifier_state(struct verifier_state *state);
void free_verifier_block(struct verifier_block *block);
void free_verifier_jump_destinations(struct verifier_jump_destinations *jmp_dests);
void free_verifier_context(struct verifier_context *ctx);

int store_vrf_lvar(struct verifier_block *b, enum vm_type vm_type, unsigned int idx);
int peek_vrf_lvar(struct verifier_block *b, enum vm_type vm_type, unsigned int idx);

int vrf_stack_size(struct verifier_stack *st);
int push_vrf_op(struct verifier_block *b, enum vm_type vm_type);
int pop_vrf_op(struct verifier_block *b, enum vm_type vm_type);
int peek_vrf_op(struct verifier_block *b, enum vm_type vm_type);

int add_jump_destination(struct verifier_jump_destinations *jd, unsigned long dest);
int add_tableswitch_destinations(struct verifier_jump_destinations *jd, const unsigned char *code, unsigned long pc, unsigned long code_size);
int add_lookupswitch_destinations(struct verifier_jump_destinations *jd, const unsigned char *code, unsigned long pc, unsigned long code_size);

int transition_verifier_stack(struct verifier_stack *stc, struct verifier_stack *stn);
int transition_verifier_local_var(struct verifier_local_var *varsc, struct verifier_local_var *varsn, int nb_vars);
int transition_verifier_state(struct verifier_state *s1, struct verifier_state *s2);

int vm_method_verify(struct vm_method *vmm);

typedef int (*verify_fn_t) (struct verifier_block *);

#define E_NOT_IMPLEMENTED		(-ENOSYS)
#define E_TYPE_CHECKING			(-1)
#define E_MALFORMED_BC			(-2)
#define E_WRONG_LOCAL_INDEX		(-3)
#define E_WRONG_CONSTANT_POOL_INDEX	(-4)
#define E_INVALID_BRANCH		(-5)
#define E_FALLING_OFF			(-6)
#define E_INVALID_EXCEPTION_HANDLER	(-7)

#define INITIAL_FOLLOWERS_SIZE	(8)

#define DECLARE_VERIFIER(name) int verify_##name(struct verifier_block *)

DECLARE_VERIFIER(aaload);
DECLARE_VERIFIER(aastore);
DECLARE_VERIFIER(aconst_null);
DECLARE_VERIFIER(aload);
DECLARE_VERIFIER(aload_n);
DECLARE_VERIFIER(anewarray);
DECLARE_VERIFIER(arraylength);
DECLARE_VERIFIER(astore);
DECLARE_VERIFIER(astore_n);
DECLARE_VERIFIER(athrow);
DECLARE_VERIFIER(baload);
DECLARE_VERIFIER(bastore);
DECLARE_VERIFIER(bipush);
DECLARE_VERIFIER(caload);
DECLARE_VERIFIER(castore);
DECLARE_VERIFIER(checkcast);
DECLARE_VERIFIER(d2f);
DECLARE_VERIFIER(d2i);
DECLARE_VERIFIER(d2l);
DECLARE_VERIFIER(dadd);
DECLARE_VERIFIER(daload);
DECLARE_VERIFIER(dastore);
DECLARE_VERIFIER(dconst_n);
DECLARE_VERIFIER(ddiv);
DECLARE_VERIFIER(dload);
DECLARE_VERIFIER(dload_n);
DECLARE_VERIFIER(dmul);
DECLARE_VERIFIER(dneg);
DECLARE_VERIFIER(drem);
DECLARE_VERIFIER(dstore);
DECLARE_VERIFIER(dstore_n);
DECLARE_VERIFIER(dsub);
DECLARE_VERIFIER(dup);
DECLARE_VERIFIER(dup2);
DECLARE_VERIFIER(dup2_x1);
DECLARE_VERIFIER(dup2_x2);
DECLARE_VERIFIER(dup_x1);
DECLARE_VERIFIER(dup_x2);
DECLARE_VERIFIER(f2d);
DECLARE_VERIFIER(f2i);
DECLARE_VERIFIER(f2l);
DECLARE_VERIFIER(fadd);
DECLARE_VERIFIER(faload);
DECLARE_VERIFIER(fastore);
DECLARE_VERIFIER(fconst_n);
DECLARE_VERIFIER(fdiv);
DECLARE_VERIFIER(fload);
DECLARE_VERIFIER(fload_n);
DECLARE_VERIFIER(fmul);
DECLARE_VERIFIER(fneg);
DECLARE_VERIFIER(frem);
DECLARE_VERIFIER(fstore);
DECLARE_VERIFIER(fstore_n);
DECLARE_VERIFIER(fsub);
DECLARE_VERIFIER(getfield);
DECLARE_VERIFIER(getstatic);
DECLARE_VERIFIER(goto);
DECLARE_VERIFIER(goto_w);
DECLARE_VERIFIER(i2b);
DECLARE_VERIFIER(i2c);
DECLARE_VERIFIER(i2d);
DECLARE_VERIFIER(i2f);
DECLARE_VERIFIER(i2l);
DECLARE_VERIFIER(i2s);
DECLARE_VERIFIER(iadd);
DECLARE_VERIFIER(iaload);
DECLARE_VERIFIER(iand);
DECLARE_VERIFIER(iastore);
DECLARE_VERIFIER(iconst_n);
DECLARE_VERIFIER(idiv);
DECLARE_VERIFIER(if_acmpeq);
DECLARE_VERIFIER(if_acmpne);
DECLARE_VERIFIER(ifeq);
DECLARE_VERIFIER(ifge);
DECLARE_VERIFIER(ifgt);
DECLARE_VERIFIER(if_icmpeq);
DECLARE_VERIFIER(if_icmpge);
DECLARE_VERIFIER(if_icmpgt);
DECLARE_VERIFIER(if_icmple);
DECLARE_VERIFIER(if_icmplt);
DECLARE_VERIFIER(if_icmpne);
DECLARE_VERIFIER(ifle);
DECLARE_VERIFIER(iflt);
DECLARE_VERIFIER(ifne);
DECLARE_VERIFIER(ifnonnull);
DECLARE_VERIFIER(ifnull);
DECLARE_VERIFIER(iinc);
DECLARE_VERIFIER(iload);
DECLARE_VERIFIER(iload_n);
DECLARE_VERIFIER(imul);
DECLARE_VERIFIER(ineg);
DECLARE_VERIFIER(instanceof);
DECLARE_VERIFIER(invokeinterface);
DECLARE_VERIFIER(invokespecial);
DECLARE_VERIFIER(invokestatic);
DECLARE_VERIFIER(invokevirtual);
DECLARE_VERIFIER(ior);
DECLARE_VERIFIER(irem);
DECLARE_VERIFIER(ishl);
DECLARE_VERIFIER(ishr);
DECLARE_VERIFIER(istore);
DECLARE_VERIFIER(istore_n);
DECLARE_VERIFIER(isub);
DECLARE_VERIFIER(iushr);
DECLARE_VERIFIER(ixor);
DECLARE_VERIFIER(jsr);
DECLARE_VERIFIER(jsr_w);
DECLARE_VERIFIER(l2d);
DECLARE_VERIFIER(l2f);
DECLARE_VERIFIER(l2i);
DECLARE_VERIFIER(ladd);
DECLARE_VERIFIER(laload);
DECLARE_VERIFIER(land);
DECLARE_VERIFIER(lastore);
DECLARE_VERIFIER(lcmp);
DECLARE_VERIFIER(lconst_n);
DECLARE_VERIFIER(ldc);
DECLARE_VERIFIER(ldc2_w);
DECLARE_VERIFIER(ldc_w);
DECLARE_VERIFIER(ldiv);
DECLARE_VERIFIER(lload);
DECLARE_VERIFIER(lload_n);
DECLARE_VERIFIER(lmul);
DECLARE_VERIFIER(lneg);
DECLARE_VERIFIER(lookupswitch);
DECLARE_VERIFIER(lor);
DECLARE_VERIFIER(lrem);
DECLARE_VERIFIER(lshl);
DECLARE_VERIFIER(lshr);
DECLARE_VERIFIER(lstore);
DECLARE_VERIFIER(lstore_n);
DECLARE_VERIFIER(lsub);
DECLARE_VERIFIER(lushr);
DECLARE_VERIFIER(lxor);
DECLARE_VERIFIER(monitorenter);
DECLARE_VERIFIER(monitorexit);
DECLARE_VERIFIER(multianewarray);
DECLARE_VERIFIER(new);
DECLARE_VERIFIER(newarray);
DECLARE_VERIFIER(nop);
DECLARE_VERIFIER(pop);
DECLARE_VERIFIER(pop2);
DECLARE_VERIFIER(putfield);
DECLARE_VERIFIER(putstatic);
DECLARE_VERIFIER(ret);
DECLARE_VERIFIER(return);
DECLARE_VERIFIER(saload);
DECLARE_VERIFIER(sastore);
DECLARE_VERIFIER(sipush);
DECLARE_VERIFIER(swap);
DECLARE_VERIFIER(tableswitch);
DECLARE_VERIFIER(wide);
DECLARE_VERIFIER(dcmpg);
DECLARE_VERIFIER(fcmpg);
DECLARE_VERIFIER(dcmpl);
DECLARE_VERIFIER(fcmpl);
DECLARE_VERIFIER(areturn);
DECLARE_VERIFIER(dreturn);
DECLARE_VERIFIER(freturn);
DECLARE_VERIFIER(ireturn);
DECLARE_VERIFIER(lreturn);

#endif
