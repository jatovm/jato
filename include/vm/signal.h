#ifndef VM_SIGNAL_H
#define VM_SIGNAL_H

/*
 * Signal bottom half handler is called with the address of faulting
 * instruction as argument. The address that handler returns is the
 * address to which controll will be transferred when it returns.
 */
typedef unsigned long (*signal_bh_fn)(unsigned long);

void setup_signal_handlers(void);
int install_signal_bh(void *ctx, signal_bh_fn bh);
unsigned long throw_from_signal_bh(unsigned long jit_addr);

/*
 * This macro should be called directly from signal bottom half
 * to obtain exception handler address.
 *
 * The frame chain looks here like this:
 *
 * 0  <signal_bottom_half_handler>
 * 1  <signal_bh_trampoline>
 * 2  <jit_method>
 *    ...
 */
#define throw_from_signal_bh(jit_addr)					\
	(unsigned long) throw_from_jit(jit_lookup_cu(jit_addr),		\
				       __builtin_frame_address(2),	\
				       (unsigned char *)jit_addr)

#endif /* VM_SIGNAL_H */
