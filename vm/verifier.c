#include "jit/bc-offset-mapping.h"

#include "jit/bytecode-to-ir.h"
#include "jit/expression.h"
#include "jit/exception.h"
#include "jit/subroutine.h"
#include "jit/statement.h"
#include "jit/tree-node.h"
#include "jit/compiler.h"

#include "cafebabe/constant_pool.h"

#include "vm/bytecode.h"
#include "vm/method.h"
#include "vm/die.h"
#include "vm/preload.h"
#include "vm/verifier.h"

#include "lib/stack.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#define verify_goto_w		verify_goto
#define verify_jsr_w		verify_jsr

#define BYTECODE(opc, name, size, type) [opc] = verify_ ## name,
static verify_fn_t verifiers[] = {
#  include <vm/bytecode-def.h>
};
#undef BYTECODE

static int verify_instruction(struct verify_context *vrf)
{
	verify_fn_t verify;

	vrf->opc = bytecode_read_u8(vrf->buffer);

	if (vrf->opc >= ARRAY_SIZE(verifiers))
		return warn("%d out of bounds", vrf->opc), -EINVAL;

	verify = verifiers[vrf->opc];
	if (!verify)
		return warn("no verifier function for %d found", vrf->opc), -EINVAL;

	int err = verify(vrf);

	if (err)
		warn("verifying error at PC=%lu", vrf->offset);

#if 0
	/* Would just produce too much output right now ! */
	if (err == E_NOT_IMPLEMENTED)
		warn("verifying not implemented for instruction at PC=%lu", vrf->offset);
#endif

	return err;
}

static int do_vm_method_verify(struct vm_method *vmm)
{
	int err;
	struct verify_context vrf;
	struct bytecode_buffer buffer;

	buffer.buffer = vmm->code_attribute.code;
	buffer.pos = 0;

	vrf.code_size = vmm->code_attribute.code_length;
	vrf.code = vmm->code_attribute.code;

	vrf.buffer = &buffer;

	while (buffer.pos < vrf.code_size) {
		vrf.offset = vrf.buffer->pos;	/* this is fragile */

		err = verify_instruction(&vrf);
		if (err)
			return err;
	}

	return 0;
}

int vm_method_verify(struct vm_method *vmm)
{
	int err;

	err = do_vm_method_verify(vmm);

	if (err == E_NOT_IMPLEMENTED)
		return 0;

	if (err)
		signal_new_exception(vm_java_lang_VerifyError, NULL);

	return err;
}
