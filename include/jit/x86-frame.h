#ifndef __X86_FRAME_H
#define __X86_FRAME_H

struct methodblock;
struct expression;

long frame_local_offset(struct methodblock *, struct expression *);

#endif
