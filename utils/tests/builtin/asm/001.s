.intel_syntax noprefix
.include "/import/header.s"
.text
.globl	main

main:
    push rbp
    mov  rbp, rsp
    sub  rsp, 16
    lea  rax, .STR1[rip]
    mov  QWORD PTR -8[rbp], rax ;// assign [str]
    mov  rdi, QWORD PTR -8[rbp] ;// assign [rdi]
    call strlen
    mov  DWORD PTR -12[rbp], eax ;// assign [a]
    mov  edi, DWORD PTR -12[rbp] ;// assign [edi]
    call putnbr
    mov  eax, 0
    leave 
    ret  
.endmain:.STR1: .string "fffff"
.section	.note.GNU-stack,"",@progbits

