

global get_regs
get_regs: ;pointer should be in RDI
    mov [rdi + 0], rax        ; gpr_state.rax = rax
    mov [rdi + 8], rbx        ; gpr_state.rbx = rbx
    mov [rdi + 16], rcx       ; gpr_state.rcx = rcx
    mov [rdi + 24], rdx       ; gpr_state.rdx = rdx
    mov [rdi + 32], rdi       ; gpr_state.rdi = rdi
    mov [rdi + 40], rsi       ; gpr_state.rsi = rsi
    mov [rdi + 48], rbp       ; gpr_state.rbp = rbp
    mov [rdi + 56], rsp       ; gpr_state.rsp = rsp
    mov rax, [rsp + 0]
    mov [rdi + 64], rax  ; gpr_state.rip = the start of the stack frame (interrupt only)
    mov [rdi + 72], r8        ; gpr_state.r8 = r8
    mov [rdi + 80], r9        ; gpr_state.r9 = r9
    mov [rdi + 88], r10       ; gpr_state.r10 = r10
    mov [rdi + 96], r11       ; gpr_state.r11 = r11
    mov [rdi + 104], r12      ; gpr_state.r12 = r12
    mov [rdi + 112], r13      ; gpr_state.r13 = r13
    mov [rdi + 120], r14      ; gpr_state.r14 = r14
    mov [rdi + 128], r15      ; gpr_state.r15 = r15
    ret




global restore_execution
restore_execution ;pointer should be in RDI
    mov rax, [rdi + 0]        ; rax = gpr_state.rax
    mov rbx, [rdi + 8]        ; rbx = gpr_state.rbx
    mov rcx, [rdi + 16]       ; rcx = gpr_state.rcx
    mov rdx, [rdi + 24]       ; rdx = gpr_state.rdx
    mov rdi, [rdi + 32]       ; rdi = gpr_state.rdi
    mov rsi, [rdi + 40]       ; rsi = gpr_state.rsi
    mov rbp, [rdi + 48]       ; rbp = gpr_state.rbp
    mov rsp, [rdi + 56]       ; rsp = gpr_state.rsp
    mov rax, [rdi + 64]       ; rax = gpr_state.rip (from saved stack frame)
    mov r8, [rdi + 72]        ; r8 = gpr_state.r8
    mov r9, [rdi + 80]        ; r9 = gpr_state.r9
    mov r10, [rdi + 88]       ; r10 = gpr_state.r10
    mov r11, [rdi + 96]       ; r11 = gpr_state.r11
    mov r12, [rdi + 104]      ; r12 = gpr_state.r12
    mov r13, [rdi + 112]      ; r13 = gpr_state.r13
    mov r14, [rdi + 120]      ; r14 = gpr_state.r14
    mov r15, [rdi + 128]      ; r15 = gpr_state.r15
    jmp rax
    
    
global context_switch ; This will likely not work as is will need to modify it. context_switch(struct gpr_state *old, struct gpr_state *new)
context_switch
        mov [rdi + 0], rax        ; gpr_state.rax = rax
        mov [rdi + 8], rbx        ; gpr_state.rbx = rbx
        mov [rdi + 16], rcx       ; gpr_state.rcx = rcx
        mov [rdi + 24], rdx       ; gpr_state.rdx = rdx
        mov [rdi + 32], rdi       ; gpr_state.rdi = rdi
        mov [rdi + 40], rsi       ; gpr_state.rsi = rsi
        mov [rdi + 48], rbp       ; gpr_state.rbp = rbp
        mov [rdi + 56], rsp       ; gpr_state.rsp = rsp
        mov rax, [rbp + 0]
        mov [rdi + 64], rax  ; gpr_state.rip = the start of the stack frame (interrupt only)
        mov [rdi + 72], r8        ; gpr_state.r8 = r8
        mov [rdi + 80], r9        ; gpr_state.r9 = r9
        mov [rdi + 88], r10       ; gpr_state.r10 = r10
        mov [rdi + 96], r11       ; gpr_state.r11 = r11
        mov [rdi + 104], r12      ; gpr_state.r12 = r12
        mov [rdi + 112], r13      ; gpr_state.r13 = r13
        mov [rdi + 120], r14      ; gpr_state.r14 = r14
        mov [rdi + 128], r15      ; gpr_state.r15 = r15
        
        mov rax, [rsi + 0]        ; rax = gpr_state.rax
        mov rbx, [rsi + 8]        ; rbx = gpr_state.rbx
        mov rcx, [rsi + 16]       ; rcx = gpr_state.rcx
        mov rdx, [rsi + 24]       ; rdx = gpr_state.rdx
        mov rdi, [rsi + 32]       ; rsi = gpr_state.rsi
        mov rsi, [rsi + 40]       ; rsi = gpr_state.rsi
        mov rbp, [rsi + 48]       ; rbp = gpr_state.rbp
        mov rsp, [rsi + 56]       ; rsp = gpr_state.rsp
        mov rax, [rsi + 64]       ; rax = gpr_state.rip (from saved stack frame)
        mov r8, [rsi + 72]        ; r8 = gpr_state.r8
        mov r9, [rsi + 80]        ; r9 = gpr_state.r9
        mov r10, [rsi + 88]       ; r10 = gpr_state.r10
        mov r11, [rsi + 96]       ; r11 = gpr_state.r11
        mov r12, [rsi + 104]      ; r12 = gpr_state.r12
        mov r13, [rsi + 112]      ; r13 = gpr_state.r13
        mov r14, [rsi + 120]      ; r14 = gpr_state.r14
        mov r15, [rsi + 128]      ; r15 = gpr_state.r15
        ret
        
        