
%macro pushaq 0
    cli
    push rax      ;save current rax
    push rbx      ;save current rbx
    push rcx      ;save current rcx
    push rdx      ;save current rdx
    push rbp      ;save current rbp
    push rdi       ;save current rdi
    push rsi       ;save current rsi
    push r8        ;save current r8
    push r9        ;save current r9
    push r10      ;save current r10
    push r11      ;save current r11
    push r12      ;save current r12
    push r13      ;save current r13
    push r14      ;save current r14
    push r15      ;save current r15

%endmacro

%macro popaq 0
    pop r15      ;save current r15
    pop r14      ;save current r14
    pop r13      ;save current r13
    pop r12      ;save current r12
    pop r11      ;save current r11
    pop r10      ;save current r10
    pop r9        ;save current r9
    pop r8        ;save current r8
    pop rsi       ;save current rsi
    pop rdi       ;save current rdi
    pop rbp      ;save current rbp
    pop rdx      ;save current rdx
    pop rcx      ;save current rcx
    pop rbx      ;save current rbx
    pop rax      ;save current rax
    sti

%endmacro


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