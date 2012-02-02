#include "sys/signal.h"

#include "vm/signal.h"

#include <assert.h>

bool signal_from_native(void *ctx)
{
	ucontext_t *uc = ctx;
	struct sigcontext *sc = &uc->uc_mcontext;

	return is_native(sc->arm_pc);
}

struct compilation_unit *get_signal_source_cu(void *ctx)
{
	ucontext_t *uc = ctx;
	struct sigcontext *sc = &uc->uc_mcontext;

	return jit_lookup_cu(sc->arm_pc);
}

int install_signal_bh(void *ctx, signal_bh_fn bh)
{
	assert(!"not implemented");
}

