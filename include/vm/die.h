#ifndef __DIE_H
#define __DIE_H

void die(const char *format, ...) __attribute__ ((noreturn))
    __attribute__ ((format (printf, 1, 2)));

#endif
