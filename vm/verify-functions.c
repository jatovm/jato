#include "vm/verifier.h"

#include "jit/statement.h"
#include "jit/compiler.h"

#include "vm/bytecode.h"
#include "vm/die.h"

#include <errno.h>

static unsigned int read_index(struct verifier_block *bb)
{
	if (bb->is_wide) {
		bb->pc += 2;
		return read_u16(bb->code);
	}

	bb->pc++;
	return read_u8(bb->code);
}

static inline void discard_bc_params(struct verifier_block *bb, unsigned int bytes)
{
	/* We need to advance PC but we have no use for the parameters here. */
	bb->pc += bytes;
}

static int verify_stack_max_size(struct verifier_block *bb, unsigned int added_size)
{
	if (bb->parent_ctx->max_stack < vrf_stack_size(bb->final_state->stack) + added_size)
		return E_TYPE_CHECKING;

	return 0;
}

static int __verify_binop(struct verifier_block *bb, enum vm_type vm_type)
{
	int err;

	err = pop_vrf_op(bb, vm_type);
	if (err)
		return err;

	err = peek_vrf_op(bb, vm_type);
	if (err)
		return err;

	return 0;
}

int verify_iadd(struct verifier_block *bb)
{
	return __verify_binop(bb, J_INT);
}

int verify_ladd(struct verifier_block *bb)
{
	return __verify_binop(bb, J_LONG);
}

int verify_fadd(struct verifier_block *bb)
{
	return __verify_binop(bb, J_FLOAT);
}

int verify_dadd(struct verifier_block *bb)
{
	return __verify_binop(bb, J_DOUBLE);
}

int verify_isub(struct verifier_block *bb)
{
	return __verify_binop(bb, J_INT);
}

int verify_lsub(struct verifier_block *bb)
{
	return __verify_binop(bb, J_LONG);
}

int verify_fsub(struct verifier_block *bb)
{
	return __verify_binop(bb, J_FLOAT);
}

int verify_dsub(struct verifier_block *bb)
{
	return __verify_binop(bb, J_DOUBLE);
}

int verify_imul(struct verifier_block *bb)
{
	return __verify_binop(bb, J_INT);
}

int verify_lmul(struct verifier_block *bb)
{
	return __verify_binop(bb, J_LONG);
}

int verify_fmul(struct verifier_block *bb)
{
	return __verify_binop(bb, J_FLOAT);
}

int verify_dmul(struct verifier_block *bb)
{
	return __verify_binop(bb, J_DOUBLE);
}

int verify_idiv(struct verifier_block *bb)
{
	return __verify_binop(bb, J_INT);
}

int verify_ldiv(struct verifier_block *bb)
{
	return __verify_binop(bb, J_LONG);
}

int verify_fdiv(struct verifier_block *bb)
{
	return __verify_binop(bb, J_FLOAT);
}

int verify_ddiv(struct verifier_block *bb)
{
	return __verify_binop(bb, J_DOUBLE);
}

int verify_irem(struct verifier_block *bb)
{
	return __verify_binop(bb, J_INT);
}

int verify_lrem(struct verifier_block *bb)
{
	return __verify_binop(bb, J_LONG);
}

int verify_frem(struct verifier_block *bb)
{
	return __verify_binop(bb, J_FLOAT);
}

int verify_drem(struct verifier_block *bb)
{
	return __verify_binop(bb, J_DOUBLE);
}

static int __verify_unary_op(struct verifier_block *bb, enum vm_type vm_type)
{
	int err;

	err = peek_vrf_op(bb, vm_type);
	if (err)
		return err;

	return 0;
}

int verify_ineg(struct verifier_block *bb)
{
	return __verify_unary_op(bb, J_INT);
}

int verify_lneg(struct verifier_block *bb)
{
	return __verify_unary_op(bb, J_LONG);
}

int verify_fneg(struct verifier_block *bb)
{
	return __verify_unary_op(bb, J_FLOAT);
}

int verify_dneg(struct verifier_block *bb)
{
	return __verify_unary_op(bb, J_DOUBLE);
}

static int __verify_xshx(struct verifier_block *bb, enum vm_type vm_type)
{
	int err;

	err = pop_vrf_op(bb, vm_type);
	if (err)
		return err;

	err = pop_vrf_op(bb, J_INT);
	if (err)
		return err;

	err = push_vrf_op(bb, vm_type);
	if (err)
		return err;

	return 0;
}

int verify_ishl(struct verifier_block *bb)
{
	return __verify_xshx(bb, J_INT);
}

int verify_lshl(struct verifier_block *bb)
{
	return __verify_xshx(bb, J_LONG);
}

int verify_ishr(struct verifier_block *bb)
{
	return __verify_xshx(bb, J_INT);
}

int verify_lshr(struct verifier_block *bb)
{
	return __verify_xshx(bb, J_LONG);
}

int verify_iushr(struct verifier_block *bb)
{
	return __verify_xshx(bb, J_INT);
}

int verify_lushr(struct verifier_block *bb)
{
	return __verify_xshx(bb, J_LONG);
}

int verify_iand(struct verifier_block *bb)
{
	return __verify_binop(bb, J_INT);
}

int verify_land(struct verifier_block *bb)
{
	return __verify_binop(bb, J_LONG);
}

int verify_ior(struct verifier_block *bb)
{
	return __verify_binop(bb, J_INT);
}

int verify_lor(struct verifier_block *bb)
{
	return __verify_binop(bb, J_LONG);
}

int verify_ixor(struct verifier_block *bb)
{
	return __verify_binop(bb, J_INT);
}

int verify_lxor(struct verifier_block *bb)
{
	return __verify_binop(bb, J_LONG);
}

int verify_iinc(struct verifier_block *bb)
{
	int err;
	unsigned int idx;

	idx = read_index(bb);

	discard_bc_params(bb, 1);

	err = peek_vrf_lvar(bb, J_INT, idx);
	if (err)
		return err;

	return 0;
}

static int __verify_xcmpx(struct verifier_block *bb, enum vm_type vm_type)
{
	int err;

	err = pop_vrf_op(bb, vm_type);
	if (err)
		return err;

	err = pop_vrf_op(bb, vm_type);
	if (err)
		return err;

	err = push_vrf_op(bb, J_INT);
	if (err)
		return err;

	return 0;
}

int verify_lcmp(struct verifier_block *bb)
{
	return __verify_xcmpx(bb, J_LONG);
}

int verify_dcmpl(struct verifier_block *bb)
{
	return __verify_xcmpx(bb, J_DOUBLE);
}

int verify_fcmpl(struct verifier_block *bb)
{
	return __verify_xcmpx(bb, J_FLOAT);
}

int verify_dcmpg(struct verifier_block *bb)
{
	return __verify_xcmpx(bb, J_DOUBLE);
}

int verify_fcmpg(struct verifier_block *bb)
{
	return __verify_xcmpx(bb, J_FLOAT);
}

static int __verify_if_binary(struct verifier_block *bb)
{
	int err;

	err = pop_vrf_op(bb, J_INT);
	if (err)
		return err;

	discard_bc_params(bb, 2);

	return 0;
}

int verify_ifeq(struct verifier_block *bb)
{
	return __verify_if_binary(bb);
}

int verify_ifne(struct verifier_block *bb)
{
	return __verify_if_binary(bb);
}

int verify_iflt(struct verifier_block *bb)
{
	return __verify_if_binary(bb);
}

int verify_ifge(struct verifier_block *bb)
{
	return __verify_if_binary(bb);
}

int verify_ifgt(struct verifier_block *bb)
{
	return __verify_if_binary(bb);
}

int verify_ifle(struct verifier_block *bb)
{
	return __verify_if_binary(bb);
}

static int __verify_if_cmp(struct verifier_block *bb, enum vm_type vm_type)
{
	int err;

	err = pop_vrf_op(bb, vm_type);
	if (err)
		return err;

	err = pop_vrf_op(bb, vm_type);
	if (err)
		return err;

	discard_bc_params(bb, 2);

	return 0;
}

int verify_if_icmpeq(struct verifier_block *bb)
{
	return __verify_if_cmp(bb, J_INT);
}

int verify_if_icmpne(struct verifier_block *bb)
{
	return __verify_if_cmp(bb, J_INT);
}

int verify_if_icmplt(struct verifier_block *bb)
{
	return __verify_if_cmp(bb, J_INT);
}

int verify_if_icmpge(struct verifier_block *bb)
{
	return __verify_if_cmp(bb, J_INT);
}

int verify_if_icmpgt(struct verifier_block *bb)
{
	return __verify_if_cmp(bb, J_INT);
}

int verify_if_icmple(struct verifier_block *bb)
{
	return __verify_if_cmp(bb, J_INT);
}

int verify_if_acmpeq(struct verifier_block *bb)
{
	return __verify_if_cmp(bb, J_REFERENCE);
}

int verify_if_acmpne(struct verifier_block *bb)
{
	return __verify_if_cmp(bb, J_REFERENCE);
}

int verify_goto(struct verifier_block *bb)
{
	if (bb->is_wide)
		discard_bc_params(bb, 4);
	else
		discard_bc_params(bb, 2);

	return 0;
}

int verify_jsr(struct verifier_block *bb)
{
	int err;

	verify_stack_max_size(bb, 1);

	err = push_vrf_op(bb, J_RETURN_ADDRESS);
	if (err)
		return err;

	if (bb->is_wide)
		discard_bc_params(bb, 4);
	else
		discard_bc_params(bb, 2);

	return 0;
}

int verify_ret(struct verifier_block *bb)
{
	unsigned int idx;

	idx = read_index(bb);

	return peek_vrf_lvar(bb, J_RETURN_ADDRESS, idx);
}

static int __verify_ifnull(struct verifier_block *bb)
{
	int err;

	err = pop_vrf_op(bb, J_REFERENCE);
	if (err)
		return err;

	discard_bc_params(bb, 2);

	return 0;
}

int verify_ifnull(struct verifier_block *bb)
{
	return __verify_ifnull(bb);
}

int verify_ifnonnull(struct verifier_block *bb)
{
	return __verify_ifnull(bb);
}

int verify_athrow(struct verifier_block *bb)
{
	int err;

	err = pop_vrf_op(bb, J_REFERENCE);
	if (err)
		return err;

	return 0;
}

static int __verify_xreturn(struct verifier_block *bb, enum vm_type vm_type)
{
	int err;

	err = peek_vrf_op(bb, vm_type);
	if (err)
		return err;

	return 0;
}

int verify_areturn(struct verifier_block *bb)
{
	return __verify_xreturn(bb, J_REFERENCE);
}

int verify_dreturn(struct verifier_block *bb)
{
	return __verify_xreturn(bb, J_DOUBLE);
}

int verify_freturn(struct verifier_block *bb)
{
	return __verify_xreturn(bb, J_FLOAT);
}

int verify_ireturn(struct verifier_block *bb)
{
	return __verify_xreturn(bb, J_INT);
}

int verify_lreturn(struct verifier_block *bb)
{
	return __verify_xreturn(bb, J_LONG);
}

int verify_return(struct verifier_block *bb)
{
	return 0;
}

int verify_invokeinterface(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_invokevirtual(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_invokespecial(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_invokestatic(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

static int __verify_const(struct verifier_block *bb, enum vm_type vm_type)
{
	int err;

	err = verify_stack_max_size(bb, 1);
	if (err)
		return err;

	err = push_vrf_op(bb, vm_type);
	if (err)
		return err;

	return 0;
}

int verify_aconst_null(struct verifier_block *bb)
{
	return __verify_const(bb, J_REFERENCE);
}

int verify_iconst_n(struct verifier_block *bb)
{
	return __verify_const(bb, J_INT);
}

int verify_lconst_n(struct verifier_block *bb)
{
	return __verify_const(bb, J_LONG);
}

int verify_fconst_n(struct verifier_block *bb)
{
	return __verify_const(bb, J_FLOAT);
}

int verify_dconst_n(struct verifier_block *bb)
{
	return __verify_const(bb, J_DOUBLE);
}

static int __verify_ipush(struct verifier_block *bb)
{
	int err;

	err = verify_stack_max_size(bb, 1);
	if (err)
		return err;

	err = push_vrf_op(bb, J_INT);
	if (err)
		return err;

	return 0;
}

int verify_bipush(struct verifier_block *bb)
{
	discard_bc_params(bb, 1);
	return __verify_ipush(bb);
}

int verify_sipush(struct verifier_block *bb)
{
	discard_bc_params(bb, 2);
	return __verify_ipush(bb);
}

int verify_ldc(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_ldc_w(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_ldc2_w(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

static int __verify_load(struct verifier_block *bb, enum vm_type vm_type, unsigned int idx)
{
	int err;

	err = verify_stack_max_size(bb, 1);
	if (err)
		return err;

	err = peek_vrf_lvar(bb, vm_type, idx);
	if (err)
		return err;

	err = push_vrf_op(bb, vm_type);
	if (err)
		return err;

	return 0;
}

static int __verify_load_read_index(struct verifier_block *bb, enum vm_type vm_type)
{
	unsigned int idx;

	idx = read_index(bb);

	return __verify_load(bb, vm_type, idx);
}

int verify_iload(struct verifier_block *bb)
{
	return __verify_load_read_index(bb, J_INT);
}

int verify_lload(struct verifier_block *bb)
{
	return __verify_load_read_index(bb, J_LONG);
}

int verify_fload(struct verifier_block *bb)
{
	return __verify_load_read_index(bb, J_FLOAT);
}

int verify_dload(struct verifier_block *bb)
{
	return __verify_load_read_index(bb, J_DOUBLE);
}

int verify_aload(struct verifier_block *bb)
{
	return __verify_load_read_index(bb, J_REFERENCE);
}

int verify_iload_n(struct verifier_block *bb)
{
	return __verify_load(bb, J_INT, bb->opc - OPC_ILOAD_0);
}

int verify_lload_n(struct verifier_block *bb)
{
	return __verify_load(bb, J_LONG, bb->opc - OPC_LLOAD_0);
}

int verify_fload_n(struct verifier_block *bb)
{
	return __verify_load(bb, J_FLOAT, bb->opc - OPC_FLOAD_0);
}

int verify_dload_n(struct verifier_block *bb)
{
	return __verify_load(bb, J_DOUBLE, bb->opc - OPC_DLOAD_0);
}

int verify_aload_n(struct verifier_block *bb)
{
	return __verify_load(bb, J_REFERENCE, bb->opc - OPC_ALOAD_0);
}

static int __verify_store(struct verifier_block *bb, enum vm_type vm_type, unsigned int idx)
{
	int err;

	err = pop_vrf_op(bb, vm_type);
	if (err)
		return err;

	err = store_vrf_lvar(bb, vm_type, idx);
	if (err)
		return err;

	return 0;
}

static int __verify_store_read_index(struct verifier_block *bb, enum vm_type vm_type)
{
	unsigned int idx;

	idx = read_index(bb);

	return __verify_store(bb, vm_type, idx);
}

int verify_istore(struct verifier_block *bb)
{
	return __verify_store_read_index(bb, J_INT);
}

int verify_lstore(struct verifier_block *bb)
{
	return __verify_store_read_index(bb, J_INT);
}

int verify_fstore(struct verifier_block *bb)
{
	return __verify_store_read_index(bb, J_INT);
}

int verify_dstore(struct verifier_block *bb)
{
	return __verify_store_read_index(bb, J_INT);
}

int verify_astore(struct verifier_block *bb)
{
	return __verify_store_read_index(bb, J_INT);
}

int verify_istore_n(struct verifier_block *bb)
{
	return __verify_store(bb, J_INT, bb->opc - OPC_ISTORE_0);
}

int verify_lstore_n(struct verifier_block *bb)
{
	return __verify_store(bb, J_LONG, bb->opc - OPC_LSTORE_0);
}

int verify_fstore_n(struct verifier_block *bb)
{
	return __verify_store(bb, J_FLOAT, bb->opc - OPC_FSTORE_0);
}

int verify_dstore_n(struct verifier_block *bb)
{
	return __verify_store(bb, J_DOUBLE, bb->opc - OPC_DSTORE_0);
}

int verify_astore_n(struct verifier_block *bb)
{
	return __verify_store(bb, J_REFERENCE, bb->opc - OPC_ASTORE_0);
}

int verify_nop(struct verifier_block *bb)
{
	return 0;
}

int verify_getstatic(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_putstatic(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_getfield(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_putfield(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

static int __verify_xaload(struct verifier_block *bb, enum vm_type vm_type)
{
	int err;

	err = pop_vrf_op(bb, J_INT);
	if (err)
		return err;

	err = pop_vrf_op(bb, J_REFERENCE);
	if (err)
		return err;

	err = push_vrf_op(bb, vm_type);
	if (err)
		return err;

	return 0;
}

int verify_iaload(struct verifier_block *bb)
{
	return __verify_xaload(bb, J_INT);
}

int verify_laload(struct verifier_block *bb)
{
	return __verify_xaload(bb, J_LONG);
}

int verify_faload(struct verifier_block *bb)
{
	return __verify_xaload(bb, J_FLOAT);
}

int verify_daload(struct verifier_block *bb)
{
	return __verify_xaload(bb, J_DOUBLE);
}

int verify_aaload(struct verifier_block *bb)
{
	return __verify_xaload(bb, J_REFERENCE);
}

int verify_baload(struct verifier_block *bb)
{
	return __verify_xaload(bb, J_BYTE);
}

int verify_caload(struct verifier_block *bb)
{
	return __verify_xaload(bb, J_CHAR);
}

int verify_saload(struct verifier_block *bb)
{
	return __verify_xaload(bb, J_SHORT);
}

static int __verify_xastore(struct verifier_block *bb, enum vm_type vm_type)
{
	int err;

	err = pop_vrf_op(bb, vm_type);
	if (err)
		return err;

	err = pop_vrf_op(bb, J_INT);
	if (err)
		return err;

	err = pop_vrf_op(bb, J_REFERENCE);
	if (err)
		return err;

	return 0;
}

int verify_iastore(struct verifier_block *bb)
{
	return __verify_xastore(bb, J_INT);
}

int verify_lastore(struct verifier_block *bb)
{
	return __verify_xastore(bb, J_LONG);
}

int verify_fastore(struct verifier_block *bb)
{
	return __verify_xastore(bb, J_FLOAT);
}

int verify_dastore(struct verifier_block *bb)
{
	return __verify_xastore(bb, J_DOUBLE);
}

int verify_aastore(struct verifier_block *bb)
{
	return __verify_xastore(bb, J_REFERENCE);
}

int verify_bastore(struct verifier_block *bb)
{
	return __verify_xastore(bb, J_BYTE);
}

int verify_castore(struct verifier_block *bb)
{
	return __verify_xastore(bb, J_CHAR);
}

int verify_sastore(struct verifier_block *bb)
{
	return __verify_xastore(bb, J_SHORT);
}

int verify_new(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_newarray(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_anewarray(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_multianewarray(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_arraylength(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_instanceof(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_checkcast(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_monitorenter(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_monitorexit(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_pop(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_pop2(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_dup(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_dup_x1(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_dup_x2(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_dup2(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_dup2_x1(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_dup2_x2(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_swap(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_tableswitch(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_lookupswitch(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_i2l(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_i2f(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_i2d(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_l2i(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_l2f(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_l2d(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_f2i(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_f2l(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_f2d(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_d2i(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_d2l(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_d2f(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_i2b(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_i2c(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_i2s(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}

int verify_wide(struct verifier_block *bb)
{
	return E_NOT_IMPLEMENTED;
}
