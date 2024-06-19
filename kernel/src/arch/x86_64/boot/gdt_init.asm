
;section .text
;    global gdt_setup, setGdt,reload_segments,reload_CS

;gdt_setup:
;gdtr DW 0 ; For limit storage
     DD 0 ; For base storage

;setGdt:
;   MOV   AX, [esp + 4]
 ;  MOV   [gdtr], AX
  ; MOV   EAX, [ESP + 8]
   ;MOV   [gdtr + 2], EAX
   ;LGDT  [gdtr]
   ;RET
reload_segments:
   ; Reload CS register:
   PUSH 0x08                 ; Push code segment to stack, 0x08 is a stand-in for your code segment
   LEA RAX, [rel .reload_CS] ; Load address of .reload_CS into RAX
   PUSH RAX                  ; Push this value to the stack
   RETFQ                     ; Perform a far return, RETFQ or LRETQ depending on syntax
.reload_CS:
   ; Reload data segment registers
   MOV   AX, 0x10 ; 0x10 is a stand-in for your data segment
   MOV   DS, AX
   MOV   ES, AX
   MOV   FS, AX
   MOV   GS, AX
   MOV   SS, AX
   RET