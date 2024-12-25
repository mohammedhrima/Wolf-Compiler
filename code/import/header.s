putstr:
	push	rbp
	mov		rbp, rsp
	sub		rsp, 16
	mov		QWORD PTR -8[rbp], rdi
	mov		rax, QWORD PTR -8[rbp]
	mov		rdi, rax
	call	strlen
	mov		rdx, rax
	mov		rax, QWORD PTR -8[rbp]
	mov		rsi, rax
	mov		edi, 1
	call	write
	nop
	leave
	ret

.INT_MIN: .string "-2147483648"
putnbr:
	push	rbp
	mov		rbp, rsp
	sub		rsp, 16
	mov		DWORD PTR -4[rbp], edi
	cmp		DWORD PTR -4[rbp], -2147483648
	jne		.putnbr0
	lea		rdi, .INT_MIN[rip]
	call	puts@PLT
	jmp		.putnbr3
.putnbr0:
	cmp		DWORD PTR -4[rbp], 0
	jns		.putnbr1
	mov		edi, 45
	call	putchar@PLT
	neg		DWORD PTR -4[rbp]
	mov		eax, DWORD PTR -4[rbp]
	mov		edi, eax
	call	putnbr
	jmp		.putnbr3
.putnbr1:
	cmp		DWORD PTR -4[rbp], 9
	jle		.putnbr2
	mov		eax, DWORD PTR -4[rbp]
	movsx	rdx, eax
	imul	rdx, rdx, 1717986919
	shr		rdx, 32
	sar		edx, 2
	sar		eax, 31
	mov		ecx, eax
	mov		eax, edx
	sub		eax, ecx
	mov		edi, eax
	call	putnbr
	mov		ecx, DWORD PTR -4[rbp]
	movsx	rax, ecx
	imul	rax, rax, 1717986919
	shr		rax, 32
	sar		eax, 2
	mov		esi, ecx
	sar		esi, 31
	sub		eax, esi
	mov		edx, eax
	mov		eax, edx
	sal		eax, 2
	add		eax, edx
	add		eax, eax
	sub		ecx, eax
	mov		edx, ecx
	mov		edi, edx
	call	putnbr
	jmp		.putnbr3
.putnbr2:
	mov		eax, DWORD PTR -4[rbp]
	add		eax, 48
	mov		edi, eax
	call	putchar@PLT
.putnbr3:
	nop
	leave
	ret

.minus_1_float:	.long	-2147483648
.ten_float: .long	1092616192
putfloat:
	push		rbp
	mov			rbp, rsp
	sub			rsp, 32
	movss		DWORD PTR -20[rbp], xmm0
	pxor		xmm0, xmm0
	comiss		xmm0, DWORD PTR -20[rbp]
	jbe			putfloat0
	mov			edi, 45
	call		putchar
	movss		xmm0, DWORD PTR -20[rbp]
	movss		xmm1, DWORD PTR .minus_1_float[rip]
	xorps		xmm0, xmm1
	movss		DWORD PTR -20[rbp], xmm0
putfloat0:	
	movss		xmm0, DWORD PTR -20[rbp]
	cvttss2si	rax, xmm0
	mov			QWORD PTR -16[rbp], rax
	mov			rax, QWORD PTR -16[rbp]
	mov			rdi, rax
	call		putnbr
	mov			edi, 46
	call		putchar
	pxor		xmm1, xmm1
	cvtsi2ss	xmm1, QWORD PTR -16[rbp]
	movss		xmm0, DWORD PTR -20[rbp]
	subss		xmm0, xmm1
	movss		DWORD PTR -4[rbp], xmm0
	mov			DWORD PTR -8[rbp], 0
	jmp			putfloat1
putfloat2:
	movss		xmm1, DWORD PTR -4[rbp]
	movss		xmm0, DWORD PTR .ten_float[rip]
	mulss		xmm0, xmm1
	movss		DWORD PTR -4[rbp], xmm0
	movss		xmm0, DWORD PTR -4[rbp]
	cvttss2si	rax, xmm0
	mov			rdi, rax
	call		putnbr
	movss		xmm0, DWORD PTR -4[rbp]
	cvttss2si	rax, xmm0
	pxor		xmm1, xmm1
	cvtsi2ss	xmm1, rax
	movss		xmm0, DWORD PTR -4[rbp]
	subss		xmm0, xmm1
	movss		DWORD PTR -4[rbp], xmm0
	add			DWORD PTR -8[rbp], 1
putfloat1:
	cmp			DWORD PTR -8[rbp], 5
	jle			putfloat2
	nop
	leave
	ret

.True: .string	"True"
.False: .string	"False"
putbool:
	push	rbp
	mov		rbp, rsp
	sub		rsp, 16
	mov		eax, edi
	mov		BYTE PTR -4[rbp], al
	cmp		BYTE PTR -4[rbp], 0
	je		putbool0
	lea		rax, .True[rip]
	mov		rdi, rax
	call	putstr
	jmp		putbool1
putbool0:
	lea		rax, .False[rip]
	mov		rdi, rax
	call	putstr
putbool1:
	mov		eax, 0
	leave
	ret


/*
.strjoin:
	push	rbp
	mov		rbp, rsp
	sub		rsp, 32
	mov		QWORD PTR -24[rbp], rdi
	mov		QWORD PTR -32[rbp], rsi
	mov		rax, QWORD PTR -24[rbp]
	mov		rdi, rax
	call	strlen
	mov		QWORD PTR -8[rbp], rax
	mov		rax, QWORD PTR -32[rbp]
	mov		rdi, rax
	call	strlen
	mov		rdx, QWORD PTR -8[rbp]
	add		rax, rdx
	add		rax, 1
	mov		rdi, rax
	call	.allocate
	mov		QWORD PTR -16[rbp], rax
	mov		rdx, QWORD PTR -24[rbp]
	mov		rax, QWORD PTR -16[rbp]
	mov		rsi, rdx
	mov		rdi, rax
	call	.strcpy
	mov		rdx, QWORD PTR -16[rbp]
	mov		rax, QWORD PTR -8[rbp]
	add		rdx, rax
	mov		rax, QWORD PTR -32[rbp]
	mov		rsi, rax
	mov		rdi, rdx
	call	.strcpy
	mov		rax, QWORD PTR -16[rbp]
	leave
	ret*/

