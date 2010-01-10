#ifndef JATO_VM_H
#define JATO_VM_H

#include "vm/opcodes.h"
#include <stdint.h>

extern char *slash2dots(char *utf8);

/* The following are atype values of the newarray bytecode:*/
#define T_BOOLEAN               4
#define T_CHAR                  5
#define T_FLOAT                 6
#define T_DOUBLE                7
#define T_BYTE                  8
#define T_SHORT                 9
#define T_INT                  10
#define T_LONG                 11

#endif /* JATO_VM_H */
