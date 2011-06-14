.global ic_start
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
