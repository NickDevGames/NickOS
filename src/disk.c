#include <stdint.h>

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

extern void read_sector();

void read_sector_c(uint8_t cylinder, uint8_t head, uint8_t sector, uint8_t disk, void* buffer) {
    // ustaw ES:BX i rejestry CX, DH, CL, DL przed wywołaniem NASM
    // w real mode: ES w segmentach, więc buffer segment = (buf>>4), offset = buf & 0xF
    uint16_t seg = ((uint32_t)buffer) >> 4;
    uint16_t off = ((uint32_t)buffer) & 0xF;

    __asm__ __volatile__ (
        "mov %%cx, %[cyl_head_sec]\n\t"
        "mov %%dh, %[head]\n\t"
        "mov %%cl, %[sector]\n\t"
        "mov %%dl, %[disk]\n\t"
        "mov %%es, %[seg]\n\t"
        "mov %%bx, %[off]\n\t"
        "call read_sector\n\t"
        :
        : [cyl_head_sec] "r" (((uint16_t)cylinder << 8) | sector),
          [head] "r" (head),
          [sector] "r" (sector),
          [disk] "r" (disk),
          [seg] "r" (seg),
          [off] "r" (off)
        : "cx", "dh", "cl", "dl", "es", "bx"
    );
}
