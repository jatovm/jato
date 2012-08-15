#ifndef JATO_JIT_LLVM_CORE_H
#define JATO_JIT_LLVM_CORE_H

struct compilation_unit;

int llvm_compile(struct compilation_unit *cu);
void llvm_init(void);

#endif
