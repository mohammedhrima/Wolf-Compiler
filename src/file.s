.intel_syntax noprefix
.include "/import/header.s"
.text
.globl	main

main:
    push rbp
    mov  rbp, rsp
    sub  rsp, 16
    mov  DWORD PTR -4[rbp], 0 ;// declare [d]
    mov  DWORD PTR -8[rbp], 0 ;// declare [x.a]
    mov  DWORD PTR -12[rbp], 0 ;// declare [x.b]
    mov  DWORD PTR -16[rbp], 0 ;// declare [x.c]
     ;// assign [x.a]
    mov  eax, 0
    leave 
    ret  
.endmain:.section	.note.GNU-stack,"",@progbits

