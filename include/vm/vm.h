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

/* XXX: Users of these definitions are likely to be buggy.  */

#define ARRAY_LEN(arrayRef)             *(uint32_t*)(arrayRef+1)
#define ARRAY_DATA(arrayRef)            ((void*)(((uint32_t*)(arrayRef+1))+1))
#define INST_DATA(objectRef)            ((uintptr_t*)(objectRef+1))

#define CLASS_ARRAY             6
#define IS_ARRAY(cb)                    (cb->state == CLASS_ARRAY)

#endif /* JATO_VM_H */
