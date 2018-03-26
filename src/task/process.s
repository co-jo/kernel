[GLOBAL read_eip]
read_eip:
    mov eax, [esp]
    ret

pushed_eip:
   push edx
   mov eax, 0x20
   out 0x20, al          ; Because we end up jumping, handler never returns
   sti
   ret

[GLOBAL perform_task_switch]
perform_task_switch:
    cli
    mov ebx, [esp]       ; Save EIP
    mov edx, [esp+0x4]   ; Passed in EIP (ARG 1)
    mov eax, [esp+0x8]   ; PAGEDIR Phys  (ARG 2)
    mov ebp, [esp+0xC]   ; EBP           (ARG 3)
    mov esp, [esp+0x10]  ; ESP           (ARG 4)
    mov cr3, eax         ; Set PDIR by loading into CR3
    cmp edx, 0x0         ;
    jne pushed_eip       ; If what we pass into EIP is non NULL (FORKED), JMP
    push ebx             ; Push saved EIP to return normally
    sti                  ;
    ret                  ;

; We need to so we can recognize where to ret to from a user fork
extern current_task

[GLOBAL save_frame]
save_frame:
    mov ecx, [esp]          ; Load saved EIP
    add ecx, 4              ; Skip to insturction after saved EIP
    mov edx, [current_task]
    mov [edx], esp
    mov [edx + 4], ebp
    mov [edx + 8], ecx
    ret

[GLOBAL ufork]
ufork:
    mov eax, 0x0            ; 0x0 = ID of syscall
    call save_frame
    int 0x80
    ret

[GLOBAL copy_page_physical]
copy_page_physical:
    push ebx              ; According to __cdecl, we must preserve the contents of EBX.
    pushf                 ; push EFLAGS, so we can pop it and reenable interrupts
                          ; later, if they were enabled anyway.
    cli                   ; Disable interrupts, so we aren't interrupted.
                          ; Load these in BEFORE we disable paging!
    mov ebx, [esp+12]     ; Source address
    mov ecx, [esp+16]     ; Destination address

    mov edx, cr0          ; Get the control register...
    and edx, 0x7fffffff   ; and...
    mov cr0, edx          ; Disable paging.

    mov edx, 1024         ; 1024*4bytes = 4096 bytes

.loop:
    mov eax, [ebx]        ; Get the word at the source address
    mov [ecx], eax        ; Store it at the dest address
    add ebx, 4            ; Source address += sizeof(word)
    add ecx, 4            ; Dest address += sizeof(word)
    dec edx               ; One less word to do
    jnz .loop

    mov edx, cr0          ; Get the control register again
    or  edx, 0x80000000   ; and...
    mov cr0, edx          ; Enable paging.

    popf                  ; Pop EFLAGS back.
    pop ebx               ; Get the original value of EBX back.
    ret
