#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

void print_trace(void)
{
	void *array[10];
	size_t size;
	char **strings;
	size_t i;

	size = backtrace(array, 10);
	strings = backtrace_symbols(array, size);

	for (i = 0; i < size; i++)
		printf("%s\n", strings[i]);

	free(strings);
}
