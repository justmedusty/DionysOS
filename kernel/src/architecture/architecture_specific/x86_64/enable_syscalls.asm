extern syscall_entry
global enable_syscalls
enable_syscalls:
    ; Enable SYSCALL/SYSRET
    mov rcx, 0xC0000080        ; IA32_EFER
    rdmsr
    bts eax, 0                 ; SCE
    wrmsr

    ; Set LSTAR
    mov rcx, 0xC0000082        ; IA32_LSTAR
    lea rax, [rel syscall_entry]
    mov rdx, rax
    shr rdx, 32
    wrmsr

    ; Set STAR
    mov rcx, 0xC0000081        ; IA32_STAR
    mov eax, 0x00000000
    mov edx, 0x00180008        ; user CS = 0x18, kernel CS = 0x08
    wrmsr

    ; Clear IF on entry
    mov rcx, 0xC0000084        ; IA32_FMASK
    mov eax, (1 << 9)
    xor edx, edx
    wrmsr

    ret