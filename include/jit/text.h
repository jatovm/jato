#ifndef JIT_TEXT_H
#define JIT_TEXT_H

#include <stdbool.h>
#include <stddef.h>

void jit_text_init(void);
void jit_text_lock(void);
void jit_text_unlock(void);
void *jit_text_ptr(void);
void jit_text_reserve(size_t size);
bool is_jit_text(void *);

#endif

