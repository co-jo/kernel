; This macro creates a stub for an IRQ - the first parameter is
; the IRQ number, the second is the ISR number it is remapped to.
%macro IRQ 2
  global irq%1
  irq%1:
    cli
    push byte 0
    push %2
    jmp irq_stub
%endmacro

IRQ   0,    32
IRQ   1,    33
IRQ   2,    34
IRQ   3,    35
IRQ   4,    36
IRQ   5,    37
IRQ   6,    38
IRQ   7,    39
IRQ   8,    40
IRQ   9,    41
IRQ  10,    42
IRQ  11,    43
IRQ  12,    44
IRQ  13,    45
IRQ  14,    46
IRQ  15,    47

extern irq_handler

; This is our common IRQ stub. It saves the processor state, sets
; up for kernel mode segments, calls the C-level fault handler,
; and finally restores the stack frame.
irq_stub:

    pusha                    ; Pushes edi,esi,ebp,esp,ebx,edx,ecx,eax
    push ds
    push es
    push fs
    push gs

    mov ax, 0x10  ; load the kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov eax, esp
    push eax

    call irq_handler

    pop eax
    pop gs
    pop fs
    pop es
    pop ds

    popa                     ; Pops edi,esi,ebp...
    add esp, 8     ; Cleans up the pushed error code and pushed ISR number
    sti
    iret           ; pops 5 things at once: CS, EIP, EFLAGS, SS, and ESP
