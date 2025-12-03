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

#define ATA2_DATA 0x170
#define ATA2_ERROR 0x171
#define ATA2_SECCOUNT 0x172
#define ATA2_LBA0 0x173
#define ATA2_LBA1 0x174
#define ATA2_LBA2 0x175
#define ATA2_DRIVE 0x176
#define ATA2_COMMAND 0x177
#define ATA2_STATUS 0x177

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
static void ata_wait_ready(void)
{
  uint8_t s;

  while ((s = inb(ATA_STATUS)) & ATA_SR_BSY)
  {
  }
}

// ===== Wait helpers =====
static void atapi_wait_ready(void)
{
  uint8_t s;

  while ((s = inb(ATA2_STATUS)) & ATA_SR_BSY)
  {
  }
}

static void ata_wait_drq(void)
{
  uint8_t s;

  do
  {
    s = inb(ATA_STATUS);
    if (s & ATA_SR_ERR)
    {
      break;
    }
    if (s & ATA_SR_DF)
    {
      break;
    }
  } while ((s & ATA_SR_BSY) || !(s & ATA_SR_DRQ));
}

static void atapi_wait_drq(void)
{
  uint8_t s;

  do
  {
    s = inb(ATA2_STATUS);
    if (s & ATA_SR_ERR)
    {
      break;
    }
    if (s & ATA_SR_DF)
    {
      break;
    }
  } while ((s & ATA_SR_BSY) || !(s & ATA_SR_DRQ));
}

// ===== IDENTIFY DEVICE =====
typedef struct
{
  char model[41];
  uint32_t sectors;
  uint16_t logical_sector_size;
  uint16_t physical_sector_size;
} ata_identify_t;

void ata_identify(ata_identify_t *info)
{
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
  if (!(word106 & 0x8000))
  {
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
  for (int i = 0; i < 20; i++)
  {
    info->model[i * 2] = data[27 + i] >> 8;
    info->model[i * 2 + 1] = data[27 + i] & 0xFF;
  }
  info->model[40] = 0;

  // Total number of 28-bit addressable sectors (words 60–61)
  info->sectors = ((uint32_t)data[61] << 16) | data[60];
}

// ===== READ SECTOR =====
void ata_read_sector(uint32_t lba, uint16_t *buffer)
{
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
void ata_write_sector(uint32_t lba, const uint16_t *buffer)
{
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

// void atapi_read_sector(uint32_t lba, uint16_t *buffer)
// {
//     terminal_writestring("A");

//     // 1. Czekaj aż BSY=0
//     ata_wait_ready();
//     terminal_writestring("B");

//     // 2. Wybierz dysk SLAVE z LBA = 1 (0xB0)
//     outb(ATA_DRIVE, 0xB0);
//     terminal_writestring("C");

//     ata_wait_ready();

//     // 3. Ustaw transfer size – 2048 bajtów (ATAPI sektor)
//     outb(0x1F2, 0);       // Sector count (low)
//     outb(0x1F3, 2048 & 0xFF);   // LBA low  = low byte transfer size
//     outb(0x1F4, 2048 >> 8);     // LBA mid  = high byte transfer size
//     outb(0x1F5, 0);             // LBA high = always 0
//     terminal_writestring("D");

//     // 4. Komenda PACKET
//     outb(ATA_COMMAND, 0xA0);
//     terminal_writestring("E");

//     // 5. Czekaj na DRQ
//     ata_wait_drq();
//     terminal_writestring("F");

//     // 6. Przygotuj pakiet Read(10)
//     uint16_t packet[6] = {0};

//     packet[0] = 0x2800;                // Opcode READ(10)
//     packet[1] = (lba >> 16) & 0xFFFF;  //
//     packet[2] = lba & 0xFFFF;          //
//     packet[3] = 0;                     // reserved
//     packet[4] = 0x0100;                // 1 sektor (big endian!)
//     packet[5] = 0;                     // control byte

//     terminal_writestring("G");

//     // 7. Wyślij pakiet (12 bajtów = 6 słów)
//     for (int i = 0; i < 6; i++)
//         outw(ATA_DATA, packet[i]);
//     terminal_writestring("H");

//     // 8. Czekaj na dane
//     ata_wait_drq();
//     terminal_writestring("I");

//     // 9. Odczytaj 2048 bajtów danych
//     for (int i = 0; i < 1024; i++)
//         buffer[i] = inw(ATA_DATA);
//     terminal_writestring("J");

//     // 10. GOTOWE
// }

void atapi_read_sector(uint32_t lba, uint16_t *buffer)
{

  // 1. Select Primary Slave device (CD-ROM)
  outb(0x176, 0xA0); // Slave, LBA = 1

  atapi_wait_ready(); // wait for BSY=0 and DRQ=0

  // 2. Set transfer size = 2048 bytes (typowy rozmiar sektora CD-ROM)
  outb(0x172, 0);           // sector count = 0
  outb(0x173, 2048 & 0xFF); // transfer size low byte
  outb(0x174, 2048 >> 8);   // transfer size high byte
  outb(0x175, 0);           // always 0

  // 3. Send PACKET command (0xA0)
  outb(0x177, 0xA0);

  atapi_wait_drq(); // wait for DRQ set (ready for packet)

  // 4. Prepare the READ(10) packet command
  uint8_t packet[12] = {0};
  packet[0] = 0x28; // READ(10)
  packet[1] = 0;    // flags
  packet[2] = (lba >> 24) & 0xFF;
  packet[3] = (lba >> 16) & 0xFF;
  packet[4] = (lba >> 8) & 0xFF;
  packet[5] = lba & 0xFF;
  packet[6] = 0; // reserved
  packet[7] = 0; // transfer length MSB
  packet[8] = 1; // transfer length LSB = 1 sektor
  packet[9] = 0; // control
  packet[10] = 0;
  packet[11] = 0;

  // wysyłaj po 2 bajty (word) od najmłodszego do najstarszego
  for (int i = 0; i < 6; i++)
  {
    uint16_t word = packet[2 * i] | (packet[2 * i + 1] << 8);
    outw(0x170, word);
  }

  atapi_wait_drq(); // wait for DRQ set (data ready)

  // 6. Read 2048 bytes = 1024 words from data port
  for (int i = 0; i < 1024; i++)
  {
    buffer[i] = inw(0x170);
  }
}

uint32_t atapi_get_disc_size()
{
  uint8_t packet[12] = {0};
  packet[0] = 0x25; // READ CAPACITY (10)

  // 1. select secondary ATAPI (usually slave)
  outb(0x176, 0xA0); // 0xA0 = device 0, 0xB0 = device 1
  atapi_wait_ready();

  // 2. Transfer 8 bytes (READ CAPACITY always returns 8)
  outb(0x172, 0); // sector count = 0
  outb(0x173, 8); // LBA low = size
  outb(0x174, 0);
  outb(0x175, 0);

  // 3. Send PACKET command 0xA0
  outb(0x177, 0xA0);
  atapi_wait_drq();

  // 4. Send packet (6 words, big-endian inside words)
  for (int i = 0; i < 6; i++)
  {
    uint16_t w = ((uint16_t)packet[i * 2] << 8) | packet[i * 2 + 1];
    outw(0x170, w);
  }

  atapi_wait_drq();

  // 5. Read 4 words = 8 bytes
  uint8_t buf[8];
  for (int i = 0; i < 4; i++)
  {
    uint16_t w = inw(0x170);
    buf[i * 2] = (w >> 8) & 0xFF;
    buf[i * 2 + 1] = (w) & 0xFF;
  }

  // Unpack: bytes 0–3 = max LBA (big endian)
  uint32_t max_lba =
      ((uint32_t)buf[0] << 24) | ((uint32_t)buf[1] << 16) | ((uint32_t)buf[2] << 8) | ((uint32_t)buf[3]);

  // bytes 4–7 = block size (should be 2048)
  uint32_t block_size =
      ((uint32_t)buf[4] << 24) | ((uint32_t)buf[5] << 16) | ((uint32_t)buf[6] << 8) | ((uint32_t)buf[7]);

  // total disc size in bytes
  return (max_lba + 1) * block_size;
}
