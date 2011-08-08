#ifndef JATO_X86_PEEPHOLE_H
#define JATO_X86_PEEPHOLE_H

struct compilation_unit;

int peephole_optimize(struct compilation_unit *cu);

#endif /* JATO_X86_PEEPHOLE_H */
