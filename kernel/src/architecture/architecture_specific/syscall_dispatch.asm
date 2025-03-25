extern system_call_dispatch
global syscall_entry
syscall_entry:
    swapgs               ; Swap kernel and user GS base (if using per-cpu structures)
    push r11             ; Save RFLAGS
    push rcx             ; Save return address
    push rbx             ; Save callee-saved registers
    push rbp
    push r12
    push r13
    push r14
    push r15

    mov rdi, rax         ; First argument: syscall number (from RAX)
    mov rsi, rsp         ; Second argument: pointer to syscall arguments (stack)

    call system_call_dispatch

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    pop rcx              ; Restore return address for sysret
    pop r11              ; Restore RFLAGS

    swapgs               ; Restore user GS base
    sysretq              ; Return to CS 3