#include "debug.c"
#include "io.c"
#include <stdint.h>

// ===== ATA ports =====
#define ATA_DATA 0x1F0
#define ATA_ERROR 0x1F1
#define ATA_SECCOUNT 0x1F2
#define ATA_LBA_LOW 0x1F3
#define ATA_LBA_MID 0x1F4
#define ATA_LBA_HIGH 0x1F5
#define ATA_DRIVE 0x1F6
#define ATA_COMMAND 0x1F7
#define ATA_STATUS 0x1F7
#define ATA_ALTSTATUS 0x3F6

// ===== ATA commands =====
#define ATA_CMD_READ_SECTORS 0x20
#define ATA_CMD_WRITE_SECTORS 0x30
#define ATA_CMD_IDENTIFY 0xEC

// ===== Status bits =====
#define ATA_SR_BSY 0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DF 0x20
#define ATA_SR_DRQ 0x08
#define ATA_SR_ERR 0x01

// ===== Wait helpers =====
static void ata_wait_ready(void) {
  uint8_t s;

  while ((s = inb(ATA_STATUS)) & ATA_SR_BSY) {
    char buf[32];
    itoa_bare(buf, 32, s, 16);
    DebugWriteString(buf);
  }
}

static void ata_wait_drq(void) {
  uint8_t s;
  do {
    s = inb(ATA_STATUS);
    char buf[32];
    itoa_bare(buf, 32, s, 16);
    DebugWriteString(buf);
  } while ((s & ATA_SR_BSY) || !(s & ATA_SR_DRQ));
}

// ===== IDENTIFY DEVICE =====
typedef struct {
  char model[41];
  uint32_t sectors;
  uint16_t logical_sector_size;
  uint16_t physical_sector_size;
} ata_identify_t;

void ata_identify(ata_identify_t *info) {
  ata_wait_ready();

  outb(ATA_DRIVE, 0xA0); // master, CHS mode ignored
  outb(ATA_SECCOUNT, 0);
  outb(ATA_LBA_LOW, 0);
  outb(ATA_LBA_MID, 0);
  outb(ATA_LBA_HIGH, 0);
  outb(ATA_COMMAND, ATA_CMD_IDENTIFY);

  uint8_t status = inb(ATA_STATUS);
  if (status == 0)
    return; // no drive

  ata_wait_drq();

  uint16_t data[256];
  for (int i = 0; i < 256; i++)
    data[i] = inw(ATA_DATA);

  // Słowo 106 - logiczny rozmiar sektora, w bajtach, jeśli bit 15=0
  uint16_t word106 = data[106];

  // Sprawdzenie bitu 15 - jeśli jest 0, to słowo zawiera rozmiar logicznego
  // sektora
  if (!(word106 & 0x8000)) {
    info->logical_sector_size = word106;
  }

  // Słowa 117 i 118 (ATA8) - rozmiary logiczny i fizyczny
  // 117 - logiczny, 118 - fizyczny (w bajtach)
  uint16_t word117 = data[117];
  uint16_t word118 = data[118];

  // Jeśli te słowa są różne od 0 i różne od 0xFFFF, to można ich użyć
  if (word117 != 0 && word117 != 0xFFFF)
    info->logical_sector_size = word117;

  if (word118 != 0 && word118 != 0xFFFF)
    info->physical_sector_size = word118;

  // Model string (words 27–46)
  for (int i = 0; i < 20; i++) {
    info->model[i * 2] = data[27 + i] >> 8;
    info->model[i * 2 + 1] = data[27 + i] & 0xFF;
  }
  info->model[40] = 0;

  // Total number of 28-bit addressable sectors (words 60–61)
  info->sectors = ((uint32_t)data[61] << 16) | data[60];
}

// ===== READ SECTOR =====
void ata_read_sector(uint32_t lba, uint16_t *buffer) {
  ata_wait_ready();

  outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
  outb(ATA_SECCOUNT, 1);
  outb(ATA_LBA_LOW, (uint8_t)(lba & 0xFF));
  outb(ATA_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
  outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
  outb(ATA_COMMAND, ATA_CMD_READ_SECTORS);

  ata_wait_drq();

  for (int i = 0; i < 256; i++)
    buffer[i] = inw(ATA_DATA);
}

// ===== WRITE SECTOR =====
void ata_write_sector(uint32_t lba, const uint16_t *buffer) {
  ata_wait_ready();

  outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
  outb(ATA_SECCOUNT, 1);
  outb(ATA_LBA_LOW, (uint8_t)(lba & 0xFF));
  outb(ATA_LBA_MID, (uint8_t)((lba >> 8) & 0xFF));
  outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
  outb(ATA_COMMAND, ATA_CMD_WRITE_SECTORS);

  ata_wait_drq();

  for (int i = 0; i < 256; i++)
    outw(ATA_DATA, buffer[i]);

  // Wait for write complete
  ata_wait_ready();
}

void atapi_read_sector(uint32_t lba, uint16_t *buffer) {
  DebugWriteString("A");
  // Ustaw drive na slave z bitami LBA = 1 (0xB0)
  outb(ATA_DRIVE, 0xB0);
  DebugWriteString("B");

  // Poczekaj, aż nie będzie zajęty
  ata_wait_ready();
  DebugWriteString("C");

  // Wysyłamy komendę PACKET
  outb(ATA_COMMAND, 0xA0);
  DebugWriteString("D");

  // Czekaj na DRQ
  ata_wait_drq();
  DebugWriteString("E");

  // Przygotuj pakiet READ(10)
  uint16_t packet[6] = {0};
  packet[0] = (0x28 << 8); // opcode 0x28 (READ(10)) + flags=0
  packet[1] = (uint16_t)((lba >> 16) & 0xFFFF);
  packet[2] = (uint16_t)(lba & 0xFFFF);
  packet[3] = 0;
  packet[4] = 1; // liczba sektorów do odczytu
  packet[5] = 0;
  DebugWriteString("F");

  // Wyślij pakiet - 12 bajtów (6 słów)
  for (int i = 0; i < 6; i++)
    outw(ATA_DATA, packet[i]);
  DebugWriteString("G");

  // Czekaj na DRQ ponownie - dane gotowe do odczytu
  ata_wait_drq();
  DebugWriteString("H");

  // Odczytaj 2048 bajtów = 1024 słowa 16-bitowe
  for (int i = 0; i < 1024; i++)
    buffer[i] = inw(ATA_DATA);
  DebugWriteString("I");

  // Możesz tu dodać sprawdzenie statusu po odczycie (opcjonalnie)
}
