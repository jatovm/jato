.global ic_start
.text

.type ic_start, @function
.func ic_start
ic_start:
	mov $0, %rax
	jmp *%rax
.endfunc
