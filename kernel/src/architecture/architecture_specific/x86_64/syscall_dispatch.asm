extern system_call_dispatch
extern panic
extern set_syscall_stack
global syscall_entry
syscall_entry:
    swapgs
    cli
    cld
    mov gs:16, rsp
    mov rsp, gs:0

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

   ; prepare C ABI
   mov rdi, rax          ; syscall number
   xor rbp, rbp
   and rsp, -16          ; align stack
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


    mov rsp, gs:16
    swapgs
    ; Clear unsafe RFLAGS bits
    and r11, ~(1<<10)      ; DF
    and r11, ~(1<<8)       ; TF
    or  r11, (1<<9)        ; IF must be 1
    ; RAX already contains return value
    sysretq              ; return to user mode