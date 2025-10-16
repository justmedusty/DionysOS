extern syscall_entry
global enable_syscalls
enable_syscalls:
    ; Set LSTAR to point to syscall handler
    mov rcx, 0xC0000082        ; LSTAR
    mov rax, [rel syscall_entry]   ; address of syscall handler, rip relative since .text is readonly cant do a normal relocation
    shr rdx, 32
    wrmsr

    ; Enable SYSCALL/SYSRET in EFER
    mov rcx, 0xC0000080        ; EFER
    rdmsr
    bts eax, 0                 ; set bit 0 (SCE)
    wrmsr

    ; Set STAR register with segment selectors
    ; STAR = ((user_cs << 48) | (kernel_cs << 32))
    mov rcx, 0xC0000081        ; STAR
    mov eax, 0x00080000        ; Kernel CS = 0x08
    mov edx, 0x00180000        ; User CS = 0x18
    wrmsr

    ret