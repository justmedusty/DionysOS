;arch_context_switch(old context, new context)
global arch_context_switch

arch_context_switch:
     mov rax, [rsp + 8]
     mov rdx, [rsp + 16]

      ;Save old callee-saved registers
     push rbp
     push rbx
     push rsi
     push rdi

       ;Switch stacks
     mov rsp, rax
     mov rdx, rsp

      ;Load new callee-saved registers
     pop rdi
     pop rsi
     pop rbx
     pop rbp
     ret

