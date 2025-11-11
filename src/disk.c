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

// ===== Status bits =====
#define ATA_SR_BSY 0x80
#define ATA_SR_DRDY 0x40
#define ATA_SR_DF 0x20
#define ATA_SR_DSC 0x10
#define ATA_SR_DRQ 0x08
#define ATA_SR_ERR 0x01

// ===== Wait until drive ready =====
static void ata_wait_ready(void) {
    while (inb(ATA_STATUS) & ATA_SR_BSY);
}

// ===== Wait for DRQ =====
static void ata_wait_drq(void) {
    uint8_t status;
    do {
        status = inb(ATA_STATUS);
    } while ((status & ATA_SR_BSY) || !(status & ATA_SR_DRQ));
}

// ===== Read one sector at given LBA =====
void ata_read_sector(uint32_t lba, uint16_t* buffer) {
    ata_wait_ready();

    // Prepare LBA registers
    outb(ATA_DRIVE, 0xE0 | ((lba >> 24) & 0x0F)); // 0xE0 = master + LBA mode
    outb(ATA_SECCOUNT, 1);                        // 1 sector
    outb(ATA_LBA_LOW,  (uint8_t)(lba & 0xFF));
    outb(ATA_LBA_MID,  (uint8_t)((lba >> 8) & 0xFF));
    outb(ATA_LBA_HIGH, (uint8_t)((lba >> 16) & 0xFF));

    // Send READ SECTORS command
    outb(ATA_COMMAND, 0x20);

    // Wait until data ready
    ata_wait_drq();

    // Read 256 words (512 bytes)
    for (int i = 0; i < 256; i++) {
        buffer[i] = inw(ATA_DATA);
    }

    // Optional: error check
    uint8_t status = inb(ATA_STATUS);
    if (status & ATA_SR_ERR) {
        uint8_t err = inb(ATA_ERROR);
        // handle error if needed
    }
}