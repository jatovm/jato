#ifndef JATO_VM_DIE_H
#define JATO_VM_DIE_H

#define warn(format, args...) do_warn("%s: warning: " format, __func__, ## args)
void do_warn(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

#define error(format, args...) do_error("%s: error: " format, __func__, ## args)
void do_error(const char *format, ...) __attribute__ ((noreturn))
    __attribute__ ((format (printf, 1, 2)));

void die(const char *format, ...) __attribute__ ((noreturn))
    __attribute__ ((format (printf, 1, 2)));

#endif /* JATO_VM_DIE_H */
