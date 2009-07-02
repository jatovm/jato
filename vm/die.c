#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#include <vm/backtrace.h>
#include <vm/die.h>

void do_warn(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);

	printf("\n");
}

void do_error(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);

	printf("\n");

	print_trace();
	abort();
}

void do_die(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vprintf(format, ap);
	va_end(ap);

	printf("\n");

	exit(EXIT_FAILURE);
}
