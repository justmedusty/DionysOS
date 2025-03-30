global syscall_stub
; syscall_stub(uint64 syscall_no, struct syscall_args args)
syscall_stub:
    mov rax, rdi         ; Move syscall_no into RAX
    mov rdi, rsi         ; Move first argument into RDI
    mov rsi, rdx         ; Move second argument into RSI
    mov rdx, rcx         ; Move third argument into RDX
    mov r10, r8          ; Move fourth argument into R10 (syscall ABI requires this)
    mov r8, r9           ; Move fifth argument into R8
    mov r9, [rsp + 8]    ; Move sixth argument into R9 (passed via stack)

    syscall              ; Perform the syscall
    ret                  ; Return to caller
