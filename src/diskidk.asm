;=============================================================================
; ATA read sectors (LBA mode) - 32-bit version
;
; @param EAX Logical Block Address of sector
; @param CL  Number of sectors to read
; @param EDI The address of buffer to put data obtained from disk
;
; @return None
;=============================================================================

extern terminal_putchar

section .text
global ata_lba_read

ata_lba_read:
    push byte 'A'
    call terminal_putchar
    add esp, 4

    pushfd              ; zachowaj flags
    and eax, 0x0FFFFFFF ; ogranicz LBA do 28 bitów
    
    push byte 'B'
    call terminal_putchar
    add esp, 4

    push eax            ; zachowaj LBA
    push ebx
    push ecx
    push edx
    push edi

    push byte 'C'
    call terminal_putchar
    add esp, 4

    mov ebx, eax        ; zapisz LBA w ebx

    push byte 'D'
    call terminal_putchar
    add esp, 4

    mov dx, 0x1F6       ; port do wysłania drive i bitów 24-27 LBA
    shr eax, 24         ; wyciągnij bit 24-27 do al
    or al, 0xE0         ; 11100000b - ustaw bit 7,6,5 (LBA mode i drive)
    out dx, al

    push byte 'E'
    call terminal_putchar
    add esp, 4

    mov dx, 0x1F2       ; port do wysłania ilości sektorów
    mov al, cl          ; ilość sektorów z cl
    out dx, al

    push byte 'F'
    call terminal_putchar
    add esp, 4

    mov dx, 0x1F3       ; port do wysłania bitów 0-7 LBA
    mov eax, ebx
    out dx, al

    push byte 'G'
    call terminal_putchar
    add esp, 4

    mov dx, 0x1F4       ; port do wysłania bitów 8-15 LBA
    mov eax, ebx
    shr eax, 8
    out dx, al

    push byte 'H'
    call terminal_putchar
    add esp, 4

    mov dx, 0x1F5       ; port do wysłania bitów 16-23 LBA
    mov eax, ebx
    shr eax, 16
    out dx, al

    push byte 'I'
    call terminal_putchar
    add esp, 4

    mov dx, 0x1F7       ; port polecenia
    mov al, 0x20        ; komenda READ SECTORS z retry
    out dx, al

    push byte 'J'
    call terminal_putchar
    add esp, 4

.still_going:
    in al, dx
    test al, 8          ; czy sektor gotowy?
    jz .still_going     ; czekaj

    mov ecx, 256        ; 256 słów = 512 bajtów (1 sektor)
    xor bx, bx
    mov bl, cl          ; mnożymy przez liczbę sektorów (cl)
    mul bx              ; eax * ebx -> edx:eax, interesuje nas eax

    mov ecx, eax        ; ustaw ilość słów do wczytania
    mov dx, 0x1F0       ; port danych
    
    rep insw            ; wczytaj słowa do [edi]

    pop edi
    pop edx
    pop ecx
    pop ebx
    pop eax
    popfd
    ret
