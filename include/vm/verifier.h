#ifndef __VM_VERIFIER_H
#define __VM_VERIFIER_H

#include "jit/compiler.h"
#include "lib/stack.h"

struct verify_operand {
	enum vm_type			vm_type;
	bool				is_fragment;		/* If it is not the first slot of a multi-slot op */
};

struct local_var {
	bool				is_initialized;
	struct verify_operand		op;
};

struct verify_context {
	struct bytecode_buffer		*buffer;
	unsigned char			*code;
	unsigned long			offset;
	unsigned long			code_size;
	unsigned char			opc;
	bool				is_wide;
};

struct verify_operand *alloc_verify_operand(enum vm_type vm_type);
int verify_op_type(struct verify_operand *op, enum vm_type vm_type);

int verify_local_var_type(struct verify_context *vrf, enum vm_type vm_type, unsigned int idx);
void set_local_var_type(struct verify_context *vrf, enum vm_type vm_type, unsigned int idx);
int store_local_var(struct verify_context *vrf, enum vm_type vm_type, unsigned int idx);

int vm_method_verify(struct vm_method *vmm);

typedef int (*verify_fn_t) (struct verify_context *);

#define E_NOT_IMPLEMENTED	(-ENOSYS)
#define E_TYPE_CHECKING		(-1)
#define E_MALFORMED_BC		(-2)

#define IS_LARGE(vm_type) vm_type_is_pair(vm_type)

#define DECLARE_VERIFIER(name) int verify_##name(struct verify_context *)

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
