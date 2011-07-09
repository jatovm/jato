#include "vm/verifier.h"
#include "vm/bytecode.h"
#include "lib/stack.h"

#include <errno.h>

static unsigned int read_index(struct verify_context *vrf)
{
	if (vrf->is_wide) {
		if (vrf->buffer->pos + 2 < vrf->code_size)
			return bytecode_read_u16(vrf->buffer);
		else
			return E_MALFORMED_BC;
	}

	if (vrf->buffer->pos + 1 < vrf->code_size)
		return bytecode_read_u8(vrf->buffer);
	else
		return E_MALFORMED_BC;
}

static int discard_bc_params(struct verify_context *vrf, unsigned int bytes)
{
	unsigned int i;

	/* We need to advance PC but we have no use for the parameters here. */
	for (i = 0; i < bytes; i++) {
		if (vrf->buffer->pos + 1 < vrf->code_size)
			return bytecode_read_u8(vrf->buffer);
		else
			return E_MALFORMED_BC;
	}

	/* Should never be reached. */
	return 0;
}

static int __verify_binop(struct verify_context *vrf, enum vm_type vm_type)
{
	return 0;
}

int verify_iadd(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_INT);
}

int verify_ladd(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_LONG);
}

int verify_fadd(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_FLOAT);
}

int verify_dadd(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_DOUBLE);
}

int verify_isub(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_INT);
}

int verify_lsub(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_LONG);
}

int verify_fsub(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_FLOAT);
}

int verify_dsub(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_DOUBLE);
}

int verify_imul(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_INT);
}

int verify_lmul(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_LONG);
}

int verify_fmul(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_FLOAT);
}

int verify_dmul(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_DOUBLE);
}

int verify_idiv(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_INT);
}

int verify_ldiv(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_LONG);
}

int verify_fdiv(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_FLOAT);
}

int verify_ddiv(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_DOUBLE);
}

int verify_irem(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_INT);
}

int verify_lrem(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_LONG);
}

int verify_frem(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_FLOAT);
}

int verify_drem(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_DOUBLE);
}

static int __verify_unary_op(struct verify_context *vrf, enum vm_type vm_type)
{
	return 0;
}

int verify_ineg(struct verify_context *vrf)
{
	return __verify_unary_op(vrf, J_INT);
}

int verify_lneg(struct verify_context *vrf)
{
	return __verify_unary_op(vrf, J_LONG);
}

int verify_fneg(struct verify_context *vrf)
{
	return __verify_unary_op(vrf, J_FLOAT);
}

int verify_dneg(struct verify_context *vrf)
{
	return __verify_unary_op(vrf, J_DOUBLE);
}

static int __verify_xshx(struct verify_context *vrf, enum vm_type vm_type)
{
	return 0;
}

int verify_ishl(struct verify_context *vrf)
{
	return __verify_xshx(vrf, J_INT);
}

int verify_lshl(struct verify_context *vrf)
{
	return __verify_xshx(vrf, J_LONG);
}

int verify_ishr(struct verify_context *vrf)
{
	return __verify_xshx(vrf, J_INT);
}

int verify_lshr(struct verify_context *vrf)
{
	return __verify_xshx(vrf, J_LONG);
}

int verify_iushr(struct verify_context *vrf)
{
	return __verify_xshx(vrf, J_INT);
}

int verify_lushr(struct verify_context *vrf)
{
	return __verify_xshx(vrf, J_LONG);
}

int verify_iand(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_INT);
}

int verify_land(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_LONG);
}

int verify_ior(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_INT);
}

int verify_lor(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_LONG);
}

int verify_ixor(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_INT);
}

int verify_lxor(struct verify_context *vrf)
{
	return __verify_binop(vrf, J_LONG);
}

int verify_iinc(struct verify_context *vrf)
{
	int err;
	int idx;

	idx = read_index(vrf);
	if (idx < 0)
		return idx;

	err = discard_bc_params(vrf, 1);
	if (err)
		return err;

	return 0;
}

static int __verify_xcmpx(struct verify_context *vrf, enum vm_type vm_type)
{
	return 0;
}

int verify_lcmp(struct verify_context *vrf)
{
	return __verify_xcmpx(vrf, J_LONG);
}

int verify_dcmpl(struct verify_context *vrf)
{
	return __verify_xcmpx(vrf, J_DOUBLE);
}

int verify_fcmpl(struct verify_context *vrf)
{
	return __verify_xcmpx(vrf, J_FLOAT);
}

int verify_dcmpg(struct verify_context *vrf)
{
	return __verify_xcmpx(vrf, J_DOUBLE);
}

int verify_fcmpg(struct verify_context *vrf)
{
	return __verify_xcmpx(vrf, J_FLOAT);
}

static int __verify_if_binary(struct verify_context *vrf)
{
	int err;

	err = discard_bc_params(vrf, 2);
	if (err)
		return err;

	return 0;
}

int verify_ifeq(struct verify_context *vrf)
{
	return __verify_if_binary(vrf);
}

int verify_ifne(struct verify_context *vrf)
{
	return __verify_if_binary(vrf);
}

int verify_iflt(struct verify_context *vrf)
{
	return __verify_if_binary(vrf);
}

int verify_ifge(struct verify_context *vrf)
{
	return __verify_if_binary(vrf);
}

int verify_ifgt(struct verify_context *vrf)
{
	return __verify_if_binary(vrf);
}

int verify_ifle(struct verify_context *vrf)
{
	return __verify_if_binary(vrf);
}

static int __verify_if_cmp(struct verify_context *vrf, enum vm_type vm_type)
{
	int err;

	err = discard_bc_params(vrf, 2);
	if (err)
		return err;

	return 0;
}

int verify_if_icmpeq(struct verify_context *vrf)
{
	return __verify_if_cmp(vrf, J_INT);
}

int verify_if_icmpne(struct verify_context *vrf)
{
	return __verify_if_cmp(vrf, J_INT);
}

int verify_if_icmplt(struct verify_context *vrf)
{
	return __verify_if_cmp(vrf, J_INT);
}

int verify_if_icmpge(struct verify_context *vrf)
{
	return __verify_if_cmp(vrf, J_INT);
}

int verify_if_icmpgt(struct verify_context *vrf)
{
	return __verify_if_cmp(vrf, J_INT);
}

int verify_if_icmple(struct verify_context *vrf)
{
	return __verify_if_cmp(vrf, J_INT);
}

int verify_if_acmpeq(struct verify_context *vrf)
{
	return __verify_if_cmp(vrf, J_REFERENCE);
}

int verify_if_acmpne(struct verify_context *vrf)
{
	return __verify_if_cmp(vrf, J_REFERENCE);
}

int verify_goto(struct verify_context *vrf)
{
	int err;

	if (vrf->is_wide)
		err = discard_bc_params(vrf, 4);
	else
		err = discard_bc_params(vrf, 2);

	if (err)
		return err;

	return 0;
}

int verify_jsr(struct verify_context *vrf)
{
	int err;

	if (vrf->is_wide)
		err = discard_bc_params(vrf, 4);
	else
		err = discard_bc_params(vrf, 2);

	if (err)
		return err;

	return 0;
}

int verify_ret(struct verify_context *vrf)
{
	int idx;

	idx = read_index(vrf);
	if (idx < 0)
		return idx;

	return 0;
}

static int __verify_ifnull(struct verify_context *vrf)
{
	int err;

	err = discard_bc_params(vrf, 2);
	if (err)
		return err;

	return 0;
}

int verify_ifnull(struct verify_context *vrf)
{
	return __verify_ifnull(vrf);
}

int verify_ifnonnull(struct verify_context *vrf)
{
	return __verify_ifnull(vrf);
}

int verify_athrow(struct verify_context *vrf)
{
	return 0;
}

static int __verify_xreturn(struct verify_context *vrf, enum vm_type vm_type)
{
	return 0;
}

int verify_areturn(struct verify_context *vrf)
{
	return __verify_xreturn(vrf, J_REFERENCE);
}

int verify_dreturn(struct verify_context *vrf)
{
	return __verify_xreturn(vrf, J_DOUBLE);
}

int verify_freturn(struct verify_context *vrf)
{
	return __verify_xreturn(vrf, J_FLOAT);
}

int verify_ireturn(struct verify_context *vrf)
{
	return __verify_xreturn(vrf, J_INT);
}

int verify_lreturn(struct verify_context *vrf)
{
	return __verify_xreturn(vrf, J_LONG);
}

int verify_return(struct verify_context *vrf)
{
	return 0;
}

int verify_invokeinterface(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_invokevirtual(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_invokespecial(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_invokestatic(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_aconst_null(struct verify_context *vrf)
{
	return 0;
}

static int __verify_const(struct verify_context *vrf, enum vm_type vm_type)
{
	return 0;
}

int verify_iconst_n(struct verify_context *vrf)
{
	return __verify_const(vrf, J_INT);
}

int verify_lconst_n(struct verify_context *vrf)
{
	return __verify_const(vrf, J_LONG);
}

int verify_fconst_n(struct verify_context *vrf)
{
	return __verify_const(vrf, J_FLOAT);
}

int verify_dconst_n(struct verify_context *vrf)
{
	return __verify_const(vrf, J_DOUBLE);
}

static int __verify_ipush(struct verify_context *vrf)
{
	return 0;
}

int verify_bipush(struct verify_context *vrf)
{
	int err;

	err = discard_bc_params(vrf, 1);
	if (err)
		return err;

	return __verify_ipush(vrf);
}

int verify_sipush(struct verify_context *vrf)
{
	int err;

	err = discard_bc_params(vrf, 2);
	if (err)
		return err;

	return __verify_ipush(vrf);
}

int verify_ldc(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_ldc_w(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_ldc2_w(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

static int __verify_load(struct verify_context *vrf, enum vm_type vm_type, int idx)
{
	return 0;
}

static int __verify_load_read_index(struct verify_context *vrf, enum vm_type vm_type)
{
	int idx;

	idx = read_index(vrf);
	if (idx < 0)
		return idx;

	return __verify_load(vrf, vm_type, idx);
}

int verify_iload(struct verify_context *vrf)
{
	return __verify_load_read_index(vrf, J_INT);
}

int verify_lload(struct verify_context *vrf)
{
	return __verify_load_read_index(vrf, J_LONG);
}

int verify_fload(struct verify_context *vrf)
{
	return __verify_load_read_index(vrf, J_FLOAT);
}

int verify_dload(struct verify_context *vrf)
{
	return __verify_load_read_index(vrf, J_DOUBLE);
}

int verify_aload(struct verify_context *vrf)
{
	return __verify_load_read_index(vrf, J_REFERENCE);
}

int verify_iload_n(struct verify_context *vrf)
{
	return __verify_load(vrf, J_INT, vrf->opc - OPC_ILOAD_0);
}

int verify_lload_n(struct verify_context *vrf)
{
	return __verify_load(vrf, J_LONG, vrf->opc - OPC_LLOAD_0);
}

int verify_fload_n(struct verify_context *vrf)
{
	return __verify_load(vrf, J_FLOAT, vrf->opc - OPC_FLOAD_0);
}

int verify_dload_n(struct verify_context *vrf)
{
	return __verify_load(vrf, J_DOUBLE, vrf->opc - OPC_DLOAD_0);
}

int verify_aload_n(struct verify_context *vrf)
{
	return __verify_load(vrf, J_REFERENCE, vrf->opc - OPC_ALOAD_0);
}

static int __verify_store(struct verify_context *vrf, enum vm_type vm_type, int idx)
{
	return 0;
}

static int __verify_store_read_index(struct verify_context *vrf, enum vm_type vm_type)
{
	int idx;

	idx = read_index(vrf);
	if (idx < 0)
		return idx;

	return __verify_store(vrf, vm_type, idx);
}

int verify_istore(struct verify_context *vrf)
{
	return __verify_store_read_index(vrf, J_INT);
}

int verify_lstore(struct verify_context *vrf)
{
	return __verify_store_read_index(vrf, J_INT);
}

int verify_fstore(struct verify_context *vrf)
{
	return __verify_store_read_index(vrf, J_INT);
}

int verify_dstore(struct verify_context *vrf)
{
	return __verify_store_read_index(vrf, J_INT);
}

int verify_astore(struct verify_context *vrf)
{
	return __verify_store_read_index(vrf, J_INT);
}

int verify_istore_n(struct verify_context *vrf)
{
	return __verify_store(vrf, J_INT, vrf->opc - OPC_ISTORE_0);
}

int verify_lstore_n(struct verify_context *vrf)
{
	return __verify_store(vrf, J_LONG, vrf->opc - OPC_LSTORE_0);
}

int verify_fstore_n(struct verify_context *vrf)
{
	return __verify_store(vrf, J_FLOAT, vrf->opc - OPC_FSTORE_0);
}

int verify_dstore_n(struct verify_context *vrf)
{
	return __verify_store(vrf, J_DOUBLE, vrf->opc - OPC_DSTORE_0);
}

int verify_astore_n(struct verify_context *vrf)
{
	return __verify_store(vrf, J_REFERENCE, vrf->opc - OPC_ASTORE_0);
}

int verify_nop(struct verify_context *vrf)
{
	return 0;
}

int verify_getstatic(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_putstatic(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_getfield(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_putfield(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

static int __verify_xaload(struct verify_context *vrf, enum vm_type vm_type)
{
	return 0;
}

int verify_iaload(struct verify_context *vrf)
{
	return __verify_xaload(vrf, J_INT);
}

int verify_laload(struct verify_context *vrf)
{
	return __verify_xaload(vrf, J_LONG);
}

int verify_faload(struct verify_context *vrf)
{
	return __verify_xaload(vrf, J_FLOAT);
}

int verify_daload(struct verify_context *vrf)
{
	return __verify_xaload(vrf, J_DOUBLE);
}

int verify_aaload(struct verify_context *vrf)
{
	return __verify_xaload(vrf, J_REFERENCE);
}

int verify_baload(struct verify_context *vrf)
{
	return __verify_xaload(vrf, J_BYTE);
}

int verify_caload(struct verify_context *vrf)
{
	return __verify_xaload(vrf, J_CHAR);
}

int verify_saload(struct verify_context *vrf)
{
	return __verify_xaload(vrf, J_SHORT);
}

static int __verify_xastore(struct verify_context *vrf, enum vm_type vm_type)
{
	return 0;
}

int verify_iastore(struct verify_context *vrf)
{
	return __verify_xastore(vrf, J_INT);
}

int verify_lastore(struct verify_context *vrf)
{
	return __verify_xastore(vrf, J_LONG);
}

int verify_fastore(struct verify_context *vrf)
{
	return __verify_xastore(vrf, J_FLOAT);
}

int verify_dastore(struct verify_context *vrf)
{
	return __verify_xastore(vrf, J_DOUBLE);
}

int verify_aastore(struct verify_context *vrf)
{
	return __verify_xastore(vrf, J_REFERENCE);
}

int verify_bastore(struct verify_context *vrf)
{
	return __verify_xastore(vrf, J_BYTE);
}

int verify_castore(struct verify_context *vrf)
{
	return __verify_xastore(vrf, J_CHAR);
}

int verify_sastore(struct verify_context *vrf)
{
	return __verify_xastore(vrf, J_SHORT);
}

int verify_new(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_newarray(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_anewarray(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_multianewarray(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_arraylength(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_instanceof(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_checkcast(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_monitorenter(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_monitorexit(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_pop(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_pop2(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_dup(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_dup_x1(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_dup_x2(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_dup2(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_dup2_x1(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_dup2_x2(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_swap(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_tableswitch(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_lookupswitch(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_i2l(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_i2f(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_i2d(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_l2i(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_l2f(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_l2d(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_f2i(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_f2l(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_f2d(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_d2i(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_d2l(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_d2f(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_i2b(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_i2c(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_i2s(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}

int verify_wide(struct verify_context *vrf)
{
	return E_NOT_IMPLEMENTED;
}
