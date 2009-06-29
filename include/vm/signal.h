#ifndef VM_SIGNAL_H
#define VM_SIGNAL_H

/*
 * Signal bottom half handler is called with the address of faulting
 * instruction as argument. The address that handler returns is the
 * address to which controll will be transfered when it returns.
 */
typedef unsigned long (*signal_bh_fn)(unsigned long);

void setup_signal_handlers(void);
int install_signal_bh(void *ctx, signal_bh_fn bh);

#endif /* VM_SIGNAL_H */
