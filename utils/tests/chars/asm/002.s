.intel_syntax noprefix
.include "/import/header.s"
.text
.globl	main

main:
    push rbp
    mov  rbp, rsp
    sub  rsp, 16
    lea  rax, .STR1[rip]
    mov  QWORD PTR -8[rbp], rax ;// assign [planet]
    .if1:
    mov  rdi, QWORD PTR -8[rbp] ;// assign [str1]
    lea  rsi, .STR2[rip] ;// assign [str2]
    call strcmp
    cmp  eax, 0
    jne   .elif2
        lea  rdi, .STR3[rip] ;// assign [str]
        call putstr
        jmp  .endif1
    .elif2:
    mov  rdi, QWORD PTR -8[rbp] ;// assign [str1]
    lea  rsi, .STR4[rip] ;// assign [str2]
    call strcmp
    cmp  eax, 0
    jne   .elif3
        lea  rdi, .STR5[rip] ;// assign [str]
        call putstr
        jmp  .endif1
    .elif3:
    mov  rdi, QWORD PTR -8[rbp] ;// assign [str1]
    lea  rsi, .STR6[rip] ;// assign [str2]
    call strcmp
    cmp  eax, 0
    jne   .else4
        lea  rdi, .STR7[rip] ;// assign [str]
        call putstr
        jmp  .endif1
    .else4:
        lea  rdi, .STR8[rip] ;// assign [str]
        call putstr
    .endif1:
    mov  eax, 0
    leave 
    ret  
.endmain:
.STR1: .string "Mars"
.STR2: .string "Earth"
.STR3: .string "🌍 Welcome home, astronaut!\n"
.STR4: .string "Mars"
.STR5: .string "👽 Greetings Martian explorer! Temperature: -80°C\n"
.STR6: .string "Jupiter"
.STR7: .string "⚡ Danger! Extreme gas giant storms detected!\n"
.STR8: .string "🚀 Unknown planet. Proceeding with caution...\n"
.STR0: .string ""
.section	.note.GNU-stack,"",@progbits

