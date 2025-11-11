; read_sector.asm

global read_sector_c

; Wejście w rejestrach:
;   CX = cylinder (CH) + sector (CL)
;   DH = head
;   DL = dysk (np. 0x80)
;   ES:BX = wskaźnik do bufora 512 bajtów

read_sector_c:
    mov ah, 0x02    ; funkcja: read sectors
    mov al, 1       ; liczba sektorów do odczytu (1)
    int 0x13        ; wywołanie BIOS
    ret
