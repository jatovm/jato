#ifndef __JIT_EMULATE_H
#define __JIT_EMULATE_H

int emulate_lcmp(long long value1, long long value2);
long long emulate_ldiv(long long value1, long long value2);
long long emulate_lrem(long long value1, long long value2);
int emulate_fcmpl(float value1, float value2);
int emulate_fcmpg(float value1, float value2);

#endif /* __JIT_EMULATE_H */
