extern system_call_dispatch
extern panic
extern set_syscall_stack
global syscall_entry
syscall_entry:
    swapgs
    mov gs:16, rsp
    mov rsp, gs:8

    cld
    push 0x23
    push gs:16
    push r11
    push 0x2b
    push rcx

    ; Save user return state FIRST
    push r11             ; user RFLAGS
    push rcx             ; user RIP

    ; Save callee-saved registers
    push rbx
    push rbp
    push r12
    push r13
    push r14

    ; Save syscall arguments
    push r9
    push r8
    push r10
    push rdx
    push rsi
    push rdi

    ; Dispatcher arguments
    mov rdi, rax      ; syscall number
    mov rsi, rdi      ; arg1
    mov rdx, rsi      ; arg2
    mov rcx, rdx      ; arg3
    mov r8,  r10      ; arg4
    mov r9,  r8       ; arg5
    xor rbp, rbp
    call system_call_dispatch

    ; Restore syscall arguments
    pop rdi
    pop rsi
    pop rdx
    pop r10
    pop r8
    pop r9

    ; Restore callee-saved registers
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    ; Restore user return state
    pop rcx              ; user RIP
    pop r11              ; user RFLAGS

    mov rsp, gs:16
    swapgs
    ; RAX already contains return value
    sysretq              ; return to user mode