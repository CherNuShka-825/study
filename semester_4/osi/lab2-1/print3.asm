section .data
    msg db "Hello world", 10
    len equ 12

section .text
    global _start

_start:
    ; write(1, msg, len)
    mov rax, 1          ; SYS_write
    mov rdi, 1          ; stdout
    mov rsi, msg        ; адрес строки
    mov rdx, len        ; длина
    syscall

    ; exit(0)
    mov rax, 60         ; SYS_exit
    mov rdi, 0          ; код 0
    syscall
