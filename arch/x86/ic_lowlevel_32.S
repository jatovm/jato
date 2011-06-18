#define VTABLE_OFFSET	0x7c

.global ic_start
.global ic_vcall_stub

.text

.type ic_start, @function
.func ic_start
ic_start:
	push	(%esp)
	push	%eax
	push	%ecx
	call	do_ic_setup
	addl	$12, %esp
	jmp	*%eax
.endfunc

.type ic_vcall_stub, @function
.func ic_vcall_stub
ic_vcall_stub:
	movl	VTABLE_OFFSET(%ecx), %ecx
	add	%eax, %ecx
	movl	(%ecx), %ecx
	jmp	*%ecx
.endfunc
