#ifndef JATO_VM_DIE_H
#define JATO_VM_DIE_H

#define warn(format, args...) do_warn("warning: " format, ## args)
void do_warn(const char *format, ...) __attribute__ ((format (printf, 1, 2)));

#define error(format, args...) do_error("error: " format, ## args)
void do_error(const char *format, ...) __attribute__ ((noreturn))
    __attribute__ ((format (printf, 1, 2)));

#define die(format, args...) do_die("" format, ## args)

void do_die(const char *format, ...) __attribute__ ((noreturn))
    __attribute__ ((format (printf, 1, 2)));

#endif /* JATO_VM_DIE_H */
