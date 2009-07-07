#ifndef JATO_X86_TEXT_H
#define JATO_X86_TEXT_H

/*
 * As per Section 3.4.1.5 ("Code Alignment") of the Intel Optimization
 * Reference Manual, make sure functions are 16-byte aligned.
 */
#define TEXT_ALIGNMENT 16

#ifdef CONFIG_X86_64
# define TEXT_MAP_FLAGS		MAP_32BIT
#else
# define TEXT_MAP_FLAGS		0
#endif

#endif /* JATO_X86_TEXT_H */
