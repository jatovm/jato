#ifndef __MMIX_CALL_H
#define __MMIX_CALL_H

#define native_call(target, args, args_count, result)	\
	do { result = 0; } while (0)

#define vm_native_call(target, args, args_count, result)	\
	do { result = 0; } while (0)

#endif /* __MMIX_CALL_H */
