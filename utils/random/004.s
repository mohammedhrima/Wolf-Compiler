	.file	"004.c"
	.intel_syntax noprefix
	.text
	.section	.rodata
.LC0:
	.string	"abcde"
	.text
	.globl	main
	.type	main, @function
main:
	push	rbp
	mov	rbp, rsp
	lea	rax, .LC0[rip]
	mov	QWORD PTR -8[rbp], rax
	mov	eax, 0
	pop	rbp
	ret
	.size	main, .-main
	.ident	"GCC: (Debian 12.2.0-14) 12.2.0"
	.section	.note.GNU-stack,"",@progbits
