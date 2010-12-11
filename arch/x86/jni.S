#include <arch/asm.h>

.global jni_trampoline
.text

#ifdef CONFIG_X86_32
.type jni_trampoline, @function
.func jni_trampoline
jni_trampoline:
	call	vm_enter_jni
	addl	$0xc, %esp		# cleanup args

	test	%eax, %eax
	jne	error

	popl	%eax			# target
	addl	$0x4, %esp		# return address

	pushl	$vm_jni_default_env

	call	*%eax
	addl	$0x4, %esp
	subl	$0x4, %esp		# new return address

	pushl	%eax			# save return value
	call	vm_leave_jni
	movl	%eax, 0x4(%esp)		# override return address
	popl	%eax			# restore return value

	ret
error:
	addl	$0x4, %esp		# target
	ret
#else
.macro SAVE_ARGS
	pushq	%rcx
	pushq	%rdx
	pushq	%rsi
	pushq	%rdi
	pushq	%r8
	pushq	%r9
	pushq	%r10
	pushq	%r11
.endm

.macro RESTORE_ARGS
	popq	%r11
	popq	%r10
	popq	%r9
	popq	%r8
	popq	%rdi
	popq	%rsi
	popq	%rdx
	popq	%rcx
.endm

.type jni_trampoline, @function
.func jni_trampoline
jni_trampoline:
	push	%rbp
	movq	%rsp, %rbp

	SAVE_ARGS
	movq	0x18(%rbp), %rdx
	xorq	%rsi, %rsi
	movl	0x10(%rbp), %esi
	movq	0x08(%rbp), %rdi
	call	vm_enter_jni
	RESTORE_ARGS

	test	%eax, %eax
	jne	error

	SAVE_ARGS
	movq	0x20(%rbp), %rax
	movq	$vm_jni_default_env, %rdi
	call	*%rax
	RESTORE_ARGS

	SAVE_ARGS
	pushq	%rax			# save return value
	call	vm_leave_jni
	RESTORE_ARGS
	movq	%rax, -0x08(%rbp)	# override return address
	popq	%rax			# restore return value

error:
	leaveq
	addq	$0x20, %rsp
	ret
#endif
