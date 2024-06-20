
global load_gdt

load_gdt:
    LGDT [rdi]
    MOV ax, 0x10
    MOV ds, ax
    MOV es, ax
    MOV fs, ax
    MOV gs, ax
    MOV ss, ax
    POP rdi
    PUSH 0x08
    PUSH rdi
    ADD rsp , 8
    IRETQ