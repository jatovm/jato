#ifndef __X86_ASM_H
#define __X86_ASM_H

#ifdef CONFIG_X86_32

#define xax		eax
#define xbx		ebx
#define xcx		ecx
#define xdx		edx
#define xsi		esi
#define xdi		edi
#define xbp		ebp
#define xsp		esp

#define PTR_SIZE	4

#else /* CONFIG_X86_32 */

#define xax		rax
#define xbx		rbx
#define xcx		rcx
#define xdx		rdx
#define xsi		rsi
#define xdi		rdi
#define xbp		rbp
#define xsp		rsp

#define PTR_SIZE	8

#endif /* CONFIG_X86_32 */

#endif /* __X86_ASM_H */
