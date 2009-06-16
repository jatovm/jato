#ifndef JATO_PERF_MAP_H
#define JATO_PERF_MAP_H

void perf_map_open(void);
void perf_map_append(const char *symbol, unsigned long addr, unsigned long size);

#endif /* JATO_PERF_MAP_H */
