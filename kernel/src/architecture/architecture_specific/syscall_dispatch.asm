extern system_call_dispatch
global syscall_entry
syscall_entry:
    swapgs               ; Swap kernel and user GS base (if using per-cpu structures)
    push r11             ; Save RFLAGS
    push rcx             ; Save return address
    push rax             ; Save syscall number
    push rbx             ; Save callee-saved registers
    push rbp
    push r12
    push r13
    push r14
    push r15

    mov rdi, rsp         ; Pass stack pointer to syscall dispatcher
    call system_call_dispatch

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx
    pop rax             ; Restore RAX (return value)

    pop rcx             ; Restore return address for sysret
    pop r11             ; Restore RFLAGS

    swapgs              ; Restore user GS base
    sysretq             ; Return to CS 3