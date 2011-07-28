/*
 * Copyright (c) 2011  Pekka Enberg
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

#include "jit/emit-code.h"

#include "jit/basic-block.h"
#include "jit/compiler.h"

#include "arch/instruction.h"

#include "lib/buffer.h"

#include <stdlib.h>

#define OPCD(x)         (x << 26)
#define LI(x)           (x <<  2)
#define AA(x)           (x <<  1)
#define LK(x)           (x <<  0)

#define BO(x)           (x << 21)
#define BI(x)           (x << 16)

#define S(x)		(x << 21)
#define CRM(x)		(x << 12)

#define SPR(x)		(x << 16)

#define D(x)		(x << 21)
#define A(x)		(x << 16)

#define SIMM(x)		(x <<  0)
#define UIMM(x)		(x <<  0)

enum {
	BO_BR_ALWAYS	= 20,	/* branch always */
};

enum {
	SPR_XER		= 1,
	SPR_LR		= 8,
	SPR_CTR		= 9,
};

static unsigned long bl(unsigned long li)
{
        return OPCD(18) | LI(li) | AA(0) | LK(1);
}

/* Branch Conditional To Count Register */
static unsigned long bcctr(unsigned char bo, unsigned char bi, unsigned char lk)
{
	return OPCD(19) | BO(bo) | BI(bi) | (528 << 1) | LK(lk);
}

static unsigned long bctr(void)
{
	return bcctr(BO_BR_ALWAYS, 0, 0);
}

static unsigned long bctrl(void)
{
	return bcctr(BO_BR_ALWAYS, 0, 1);
}

/* Move to Special-Purpose Register */
static unsigned long mtspr(unsigned char spr, unsigned char rs)
{
	return OPCD(31) | S(rs) | SPR(spr) | (467 << 1);
}

static unsigned long mtctr(unsigned long rs)
{
	return mtspr(SPR_CTR, rs);
}

/* Add Immediate Shifted */
static unsigned long addis(unsigned char rd, unsigned char ra, unsigned short simm)
{
	return OPCD(15) | D(rd) | A(ra) | SIMM(simm);
}

static unsigned long lis(unsigned char rd, unsigned short value)
{
	return addis(rd, 0, value);
}

/* OR Immediate */
static unsigned long ori(unsigned char ra, unsigned char rs, unsigned short uimm)
{
	return OPCD(24) | S(rs) | A(ra) | UIMM(uimm);
}

static unsigned short ptr_high(void *p)
{
	unsigned long x = (unsigned long) p;

	return x >> 16;
}

static unsigned short ptr_low(void *p)
{
	unsigned long x = (unsigned long) p;

	return x & 0xffff;
}

static inline void emit8(struct buffer *buf, unsigned char c)
{
	int err;

	err = append_buffer(buf, c);

	assert(!err);
}

static void emit(struct buffer *b, unsigned long insn)
{
	union {
		unsigned long	val;
		unsigned char	b[4];
	} buf;

	buf.val = insn;
	emit8(b, buf.b[0]);
	emit8(b, buf.b[1]);
	emit8(b, buf.b[2]);
	emit8(b, buf.b[3]);
}

void itable_resolver_stub_error(struct vm_method *method, struct vm_object *obj)
{
	assert(!"not implemented");
}

int lir_print(struct insn *insn, struct string *s)
{
	assert(!"not implemented");
}

void native_call(struct vm_method *method, void *target, unsigned long *args, union jvalue *result)
{
	assert(!"not implemented");
}

void *emit_itable_resolver_stub(struct vm_class *vmc, struct itable_entry **sorted_table, unsigned int nr_entries)
{
	assert(!"not implemented");
}

bool called_from_jit_trampoline(struct native_stack_frame *frame)
{
	assert(!"not implemented");
}

bool show_exe_function(void *addr, struct string *str)
{
	assert(!"not implemented");
}

int fixup_static_at(unsigned long addr)
{
	assert(!"not implemented");
}

void
emit_trampoline(struct compilation_unit *cu, void *target_addr, struct jit_trampoline *t)
{
	struct buffer *b = t->objcode;

	jit_text_lock();

	b->buf = jit_text_ptr();

	/* Pass pointer to 'struct compilation_unit' as first argument */
	emit(b, lis(3, ptr_high(cu)));
	emit(b, ori(3, 3, ptr_low(cu)));

	/* Then call 'target_addr' */
	emit(b, lis(0, ptr_high(target_addr)));
	emit(b, ori(0, 0, ptr_low(target_addr)));
	emit(b, mtctr(0));
	emit(b, bctrl());

	/* Finally jump to the compiled method */
	emit(b, mtctr(3));
	emit(b, bctr());

	jit_text_reserve(buffer_offset(b));

	jit_text_unlock();
}

void emit_jni_trampoline(struct buffer *b, struct vm_method *vm, void *v)
{
	assert(!"not implemented");
}

void fixup_direct_calls(struct jit_trampoline *trampoline, unsigned long target)
{
	assert(!"not implemented");
}

void emit_unlock(struct buffer *buffer, struct vm_object *vo)
{
	assert(!"not implemented");
}

void emit_unlock_this(struct buffer *buffer)
{
	assert(!"not implemented");
}

void backpatch_branch_target(struct buffer *buf, struct insn *insn,
				    unsigned long target_offset)
{
	assert(!"not implemented");
}

void emit_nop(struct buffer *buf)
{
	assert(!"not implemented");
}

void emit_prolog(struct buffer *buf, struct stack_frame *frame,
					unsigned long frame_size)
{
	assert(!"not implemented");
}

void emit_epilog(struct buffer *buf)
{
	assert(!"not implemented");
}

void emit_lock(struct buffer *buf, struct vm_object *vo)
{
	assert(!"not implemented");
}

void emit_lock_this(struct buffer *buf)
{
	assert(!"not implemented");
}

void emit_unwind(struct buffer *buf)
{
	assert(!"not implemented");
}

void emit_trace_invoke(struct buffer *buf, struct compilation_unit *cu)
{
	assert(!"not implemented");
}

long branch_rel_addr(struct insn *insn, unsigned long target_offset)
{
	assert(!"not implemented");
}

long emit_branch(struct insn *insn, struct basic_block *bb)
{
	assert(!"not implemented");
}

void emit_insn(struct buffer *buf, struct basic_block *bb, struct insn *insn)
{
	assert(!"not implemented");
}
