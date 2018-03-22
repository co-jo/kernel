[GLOBAL read_eip]
read_eip:
    mov eax, [esp]
    ret

[GLOBAL perform_task_switch]
perform_task_switch:
    cli                ;
    mov eax, [esp+4]   ; physical address of current directory
    mov ebp, [esp+8]   ; EBP
    mov esp, [esp+12]  ; ESP
    mov cr3, eax       ; set the page directory
    ;mov eax, 0x12345  ;
    sti                ;
    ret                ;

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
