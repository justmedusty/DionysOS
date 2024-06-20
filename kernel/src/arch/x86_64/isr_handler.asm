extern trap
global isr_wrapper
align 16

isr_wrapper:
    pushaq
    cld
    mov rdi, rsp     ; Pass the stack pointer as argument to `trap` function
    call trap        ; Call your trap handler function
    popaq
    iretq            ; Return from interrupt