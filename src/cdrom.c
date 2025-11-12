#include "debug.c"
#include "io.c"
#include "term.c"
#include <stdint.h>

// Handy register number defines
#define DATA 0
#define ERROR_R 1
#define SECTOR_COUNT 2
#define LBA_LOW 3
#define LBA_MID 4
#define LBA_HIGH 5
#define DRIVE_SELECT 6
#define COMMAND_REGISTER 7

// Control register defines
#define CONTROL 0x206

#define ALTERNATE_STATUS 0

static __inline void insw(uint16_t __port, void *__buf, unsigned long __n) {
  __asm__ __volatile__("cld; rep; insw" : "+D"(__buf), "+c"(__n) : "d"(__port));
}

static __inline__ void outsw(uint16_t __port, const void *__buf,
                             unsigned long __n) {
  __asm__ __volatile__("cld; rep; outsw"
                       : "+S"(__buf), "+c"(__n)
                       : "d"(__port));
}

// This code is to wait 400 ns
static void ata_io_wait(const uint8_t p) {
  inb(p + CONTROL + ALTERNATE_STATUS);
  inb(p + CONTROL + ALTERNATE_STATUS);
  inb(p + CONTROL + ALTERNATE_STATUS);
  inb(p + CONTROL + ALTERNATE_STATUS);
}

// Reads sectors starting from lba to buffer
int read_cdrom(uint16_t port, bool slave, uint32_t lba, uint32_t sectors,
               uint16_t *buffer) {
  // The command
  volatile uint8_t read_cmd[12] = {0xA8,
                                   0,
                                   (lba >> 0x18) & 0xFF,
                                   (lba >> 0x10) & 0xFF,
                                   (lba >> 0x08) & 0xFF,
                                   (lba >> 0x00) & 0xFF,
                                   (sectors >> 0x18) & 0xFF,
                                   (sectors >> 0x10) & 0xFF,
                                   (sectors >> 0x08) & 0xFF,
                                   (sectors >> 0x00) & 0xFF,
                                   0,
                                   0};

  // Poprawione ustawienie drive
  outb(port + DRIVE_SELECT, 0xA0 | (slave ? 0x10 : 0x00));
  ata_io_wait(port);
  outb(port + ERROR_R, 0x00);
  // Usuń ustawianie LBA_MID i LBA_HIGH tutaj

  outb(port + COMMAND_REGISTER, 0xA0); // Packet command
  ata_io_wait(port);

  // Czekaj na DRQ lub błąd
  while (1) {
    uint8_t status = inb(port + COMMAND_REGISTER);
    char buf[2];
    itoa_bare(buf, 2, status, 16);
    DebugWriteString(buf);
    if (status & 0x01) // ERR bit
      return 1;
    if (!(status & 0x80) && (status & 0x08)) // BSY=0, DRQ=1
      break;
    ata_io_wait(port);
  }

  // Wyślij pakiet
  outsw(port + DATA, (uint16_t *)read_cmd, 6);

  // Odczytaj dane sektor po sektorze
  for (uint32_t i = 0; i < sectors; i++) {
    while (1) {
      uint8_t status = inb(port + COMMAND_REGISTER);
      if (status & 0x01)
        return 1;
      if (!(status & 0x80) && (status & 0x08))
        break;
      ata_io_wait(port);
    }

    // Sprawdź size — może być 2048 lub inna wartość
    int size = (inb(port + LBA_HIGH) << 8) | inb(port + LBA_MID);
    if (size == 0)
      size = 2048; // na wszelki wypadek

    insw(port + DATA, (uint16_t *)((uint8_t *)buffer + i * 2048), size / 2);
  }

  return 0;
}