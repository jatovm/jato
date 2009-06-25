#ifndef VM_SIGNAL_H
#define VM_SIGNAL_H

typedef void (*signal_bh_fn)(void);

void setup_signal_handlers(void);
int install_signal_bh(void *ctx, signal_bh_fn bh);

#endif /* VM_SIGNAL_H */
