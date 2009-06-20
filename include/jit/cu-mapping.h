#ifndef _JIT_CU_MAPPING
#define _JIT_CU_MAPPING

struct compilation_unit;

int add_cu_mapping(unsigned long addr, struct compilation_unit *cu);
void remove_cu_mapping(unsigned long addr);
struct compilation_unit *get_cu_from_native_addr(unsigned long addr);
void init_cu_mapping();

#endif
