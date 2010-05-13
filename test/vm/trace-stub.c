#include "vm/trace.h"

#include <stdarg.h>
#include <stdio.h>

int trace_printf(const char *fmt, ...)
{
	va_list args;
	int ret;

	va_start(args, fmt);
	ret = vprintf(fmt, args);
	va_end(args);

	return ret;
}

void trace_flush(void)
{
}
