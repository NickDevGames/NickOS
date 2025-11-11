global read_sector_stub

global cylinder
global head
global sector
global disk
global buffer_seg
global buffer_off

section .data
    ; globalne miejsce na parametry
    cylinder  dw 0
    head      db 0
    sector    db 0
    disk      db 0
    buffer_seg dw 0
    buffer_off dw 0

section .text

read_sector_stub:
    ; Załaduj parametry z pamięci do rejestrów
    mov cx, [cylinder]
    mov dh, [head]
    mov cl, [sector]
    mov dl, [disk]
    mov bx, [buffer_off]
    mov ax, [buffer_seg]
    mov es, ax

    mov ah, 0x02
    mov al, 1
    int 0x13

    ret
