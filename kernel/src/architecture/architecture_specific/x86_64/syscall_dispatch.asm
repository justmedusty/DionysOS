extern system_call_dispatch
extern panic
extern set_syscall_stack
global syscall_entry
syscall_entry:
    push rsp
    call set_syscall_stack
    push r11             ; Save RFLAGS
    push rcx             ; Save return address
    push rbx             ; Save callee-saved registers
    push rbp
    push r12
    push r13
    push r14
    push r15

    ;move args into place to be passed to the dispatcher
    push r9
    push r8
    push r10
    push rdx
    push rsi
    push rdi


    mov rdi, rax         ; First argument: syscall number (from RAX) , this overwrites the value passed but we pushed onto the stack so it is ok
    mov rsi, rsp         ; Second argument: pointer to syscall arguments (stack)

    call system_call_dispatch

    pop rdi
    pop rsi
    pop rdx
    pop r10
    pop r8
    pop r9

    pop r15
    pop r14
    pop r13
    pop r12
    pop rbp
    pop rbx

    pop rcx              ; Restore return address for sysret
    pop r11              ; Restore RFLAGS
    pop rsp

    push 0x23
    push rsp
    push r11
    push 0x1b
    push rcx

    iretq             ; Return to CS 3