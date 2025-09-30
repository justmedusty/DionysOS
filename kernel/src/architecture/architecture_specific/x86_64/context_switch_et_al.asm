

global context_switch             ;context_switch(struct register_state *old, struct register_state *new, bool user_proc, void *page_table)
context_switch

        mov [rdi + 0], rax        ; register_state.rax = rax
        mov [rdi + 8], rbx        ; register_state.rbx = rbx
        mov [rdi + 16], rcx       ; register_state.rcx = rcx
        mov [rdi + 24], rdx       ; register_state.rdx = rdx
        mov [rdi + 32], rdi       ; register_state.rdi = rdi
        mov [rdi + 40], rsi       ; register_state.rsi = rsi
        mov [rdi + 48], rbp       ; register_state.rbp = rbp
        mov [rdi + 56], rsp       ; register_state.rsp = rsp
        pop rax                   ; pop the return address off the stack
        mov [rdi + 64], rax       ; register_state.rip = the start of the stack frame (interrupt only)
        mov [rdi + 72], r8        ; register_state.r8 = r8
        mov [rdi + 80], r9        ; register_state.r9 = r9
        mov [rdi + 88], r10       ; register_state.r10 = r10
        mov [rdi + 96], r11       ; register_state.r11 = r11
        mov [rdi + 104], r12      ; register_state.r12 = r12
        mov [rdi + 112], r13      ; register_state.r13 = r13

                                  ; save the interrupt flag
        pushfq                    ;push flag onto stack
        pop rax                   ; pop flags into rax
        shl rax, 9                ; bring flag bit over
        and rax, 1                ; isolate flag bit
        mov [rdi + 136], rax      ; move the flag bit into the struct

        mov cr3, rcx                  ;load the new page table
        mov r15, rdx

        mov rbx, [rsi + 8]        ; rbx = register_state.rbx
        mov rcx, [rsi + 16]       ; rcx = register_state.rcx
        mov rdx, [rsi + 24]       ; rdx = register_state.rdx
        mov rdi, [rsi + 32]       ; rsi = register_state.rsi
        mov rbp, [rsi + 48]       ; rbp = register_state.rbp
        mov rsp, [rsi + 56]       ; rsp = register_state.rsp
        mov rax, [rsi + 64]       ; it is fine to overwrite this below because
        push rax                  ; push the RIP (return address) onto the stack to be jumped to on ret instruction
        mov r8, [rsi + 72]        ; r8 = register_state.r8
        mov r9, [rsi + 80]        ; r9 = register_state.r9
        mov r10, [rsi + 88]       ; r10 = register_state.r10
        mov r11, [rsi + 96]       ; r11 = register_state.r11
        mov r12, [rsi + 104]      ; r12 = register_state.r12
        mov r13, [rsi + 112]      ; r13 = register_state.r13

                                  ; restore the interrupt flag
        pushfq
        pop rax
        shl rax, 9                ; bring flag bit over
        and rax, 1                ; isolate flag bit
        cmp rax, [rsi + 136]      ; check if current flag bit matches
        je done                   ;if so jmp to done and finish context save by writing over rsi
        jg off                    ; if rax is greater, jump to turn interrupts off
        jmp on                    ; else turn them on


        off:
        cli                       ; clear interrupt flag
        jmp done

        on:
        sti                       ; set interrupt flag
        jmp done                  ; just for consisentency its not needed



        done:
        mov rax, [rsi + 0]        ; rax = register_state.rax; since we use rax for IF shenanigans above,restore it after
        mov rsi, [rsi + 40]       ; rsi = register_state.rsi this has to be done last to preserve the pointer argument for flag restoration

        cmp r15, 1
        jne kernel


        pop rcx                   ;sysretq expects user ret addr to be in rcx
        mov rsp, rbp
        xor rbp, rbp

        mov rax, 0x1B
        mov ds, ax
        mov ax, es
        push rax
        push rsp
        push 0x202
        push 0x23
        push rcx

        iretq


        kernel:
        ret
        
