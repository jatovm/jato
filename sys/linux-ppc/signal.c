#include "sys/signal.h"

#include "vm/signal.h"

#include <assert.h>

bool signal_from_native(void *ctx)
{
	assert(!"not implemented");
}

int install_signal_bh(void *ctx, signal_bh_fn bh)
{
	assert(!"not implemented");
}

struct compilation_unit *get_signal_source_cu(void *ctx)
{
	assert(!"not implemented");
}
