.global ic_start
.global ic_vcall_stub

.text

.type ic_start, @function
.func ic_start
ic_start:
	mov $0, %rax
	jmp *%rax
.endfunc

.type ic_vcall_stub, @function
.func ic_vcall_stub
ic_vcall_stub:
	mov $0, %rax
	jmp *%rax
.endfunc
