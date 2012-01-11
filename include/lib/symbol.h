#ifndef JATO_LIB_SYMBOL_H
#define JATO_LIB_SYMBOL_H

#include <stdbool.h>

bool symbol_init(void);
void symbol_exit(void);
char *symbol_lookup(unsigned long addr, char *buf, unsigned long len);

#endif /* JATO_LIB_SYMBOL_H */
