#ifndef JATO_VM_ERRORS_H
#define JATO_VM_ERRORS_H

#include "jit/exception.h"
#include "vm/preload.h"

static inline void *rethrow_exception(void)
{
	/* TODO: Warn if there's no exception to rethrow */

	return NULL;
}

static inline void *throw_internal_error(void)
{
	signal_new_exception(vm_java_lang_InternalError, NULL);

	return NULL;
}

static inline void *throw_chained_internal_error(void)
{
	signal_new_chained_exception(vm_java_lang_InternalError, NULL);

	return NULL;
}

static inline void *throw_oom_error(void)
{
	signal_new_exception(vm_java_lang_OutOfMemoryError, NULL);

	return NULL;
}

static inline void *throw_npe(void)
{
	signal_new_exception(vm_java_lang_NullPointerException, NULL);

	return NULL;
}

#endif
