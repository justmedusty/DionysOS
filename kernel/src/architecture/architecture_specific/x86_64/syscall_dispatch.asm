extern system_call_dispatch
extern panic
extern set_syscall_stack
global syscall_entry
syscall_entry:
    cli
    ; Save user return state FIRST
    push r11             ; user RFLAGS
    push rcx             ; user RIP

    ; Save callee-saved registers
    push rbx
    push rbp
    push r12
    push r13
    push r14
    push r15

    ; Save syscall arguments
    push r9
    push r8
    push r10
    push rdx
    push rsi
    push rdi

    ; Dispatcher arguments
    mov rdi, rax         ; syscall number
    mov rsi, rsp         ; pointer to saved arguments

    call system_call_dispatch

    ; Restore syscall arguments
    pop rdi
    pop rsi
    pop rdx
    pop r10
    pop r8
    pop r9

    ; Restore callee-saved registers
    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    ; Restore user return state
    pop rcx              ; user RIP
    pop r11              ; user RFLAGS

    ; RAX already contains return value
    sti
    sysretq              ; return to user mode