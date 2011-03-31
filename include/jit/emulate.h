#ifndef __JIT_EMULATE_H
#define __JIT_EMULATE_H

#include <stdint.h>

int emulate_lcmp(long long value1, long long value2);
long long emulate_ldiv(long long value1, long long value2);
long long emulate_lrem(long long value1, long long value2);
int64_t emulate_lshl(int64_t value1, int32_t value2);
int64_t emulate_lshr(int64_t value1, int32_t value2);
int64_t emulate_lushr(int64_t value1, int32_t value2);
int emulate_fcmpl(float value1, float value2);
int emulate_fcmpg(float value1, float value2);
int emulate_dcmpl(double value1, double value2);
int emulate_dcmpg(double value1, double value2);
int32_t emulate_f2i(float value);
int64_t emulate_f2l(float value);
int32_t emulate_d2i(double value);
int64_t emulate_d2l(double value);

#endif /* __JIT_EMULATE_H */
