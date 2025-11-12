#include "io.c"
#include <stdint.h>

// ===== ATA ports =====
#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SECCOUNT    0x1F2
#define ATA_LBA_LOW     0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HIGH    0x1F5
#define ATA_DRIVE       0x1F6
#define ATA_COMMAND     0x1F7
#define ATA_STATUS      0x1F7
#define ATA_ALTSTATUS   0x3F6

// ===== ATA commands =====
#define ATA_CMD_READ_SECTORS   0x20
#define ATA_CMD_WRITE_SECTORS  0x30
#define ATA_CMD_IDENTIFY       0xEC

// ===== Status bits =====
#define ATA_SR_BSY  0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DF   0x20
#define ATA_SR_DRQ  0x08
#define ATA_SR_ERR  0x01

// ===== Wait helpers =====
static void ata_wait_ready(void) {
    while (inb(ATA_STATUS) & ATA_SR_BSY);
}

static void ata_wait_drq(void) {
    uint8_t s;
    do {
        s = inb(ATA_STATUS);
    } while ((s & ATA_SR_BSY) || !(s & ATA_SR_DRQ));
}

// ===== IDENTIFY DEVICE =====
typedef struct {
    char model[41];
    uint32_t sectors;
} ata_identify_t;

void ata_identify(ata_identify_t* info) {
    ata_wait_ready();

    outb(ATA_DRIVE, 0xA0); // master, CHS mode ignored
    outb(ATA_SECCOUNT, 0);
    outb(ATA_LBA_LOW, 0);
    outb(ATA_LBA_MID, 0);
    outb(ATA_LBA_HIGH, 0);
    outb(ATA_COMMAND, ATA_CMD_IDENTIFY);

    uint8_t status = inb(ATA_STATUS);
    if (status == 0) return; // no drive

    ata_wait_drq();

    uint16_t data[256];
    for (int i = 0; i < 256; i++) data[i] = inw(ATA_DATA);

    // Model string (words 27–46)
    for (int i = 0; i < 20; i++) {
        info->model[i*2]     = data[27 + i] >> 8;
        info->model[i*2 + 1] = data[27 + i] & 0xFF;
    }
    info->model[40] = 0;

    // Total number of 28-bit addressable sectors (words 60–61)
    info->sectors = ((uint32_t)data[61] << 16) | data[60];
}

// ===== READ SECTOR =====
void ata_read_sector(uint32_t lba, uint16_t* buffer) {
    ata_wait_ready();

    outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECCOUNT, 1);
    outb(ATA_LBA_LOW,  (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID,  (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND, ATA_CMD_READ_SECTORS);

    ata_wait_drq();

    for (int i = 0; i < 256; i++)
        buffer[i] = inw(ATA_DATA);
}

// ===== WRITE SECTOR =====
void ata_write_sector(uint32_t lba, const uint16_t* buffer) {
    ata_wait_ready();

    outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F));
    outb(ATA_SECCOUNT, 1);
    outb(ATA_LBA_LOW,  (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID,  (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));
    outb(ATA_COMMAND, ATA_CMD_WRITE_SECTORS);

    ata_wait_drq();

    for (int i = 0; i < 256; i++)
        outw(ATA_DATA, buffer[i]);

    // Wait for write complete
    ata_wait_ready();
}