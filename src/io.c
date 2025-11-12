#pragma once

#include <stdbool.h>
#include <stdint.h>

static inline unsigned char inb(unsigned short port) {
  unsigned char result;
  __asm__ volatile("inb %1, %0" : "=a"(result) : "dN"(port));
  return result;
}

static inline void outb(unsigned short port, unsigned char data) {
  __asm__ volatile("outb %0, %1" : : "a"(data), "dN"(port));
}

// Read 16 bits from an I/O port
static inline unsigned short inw(unsigned short port) {
  unsigned short result;
  __asm__ volatile("inw %1, %0"
                   : "=a"(result) // wynik w rejestrze AX
                   : "dN"(port)); // port w DX
  return result;
}

// Write 16 bits to an I/O port
static inline void outw(unsigned short port, unsigned short data) {
  __asm__ volatile("outw %0, %1"
                   :
                   : "a"(data), "dN"(port)); // dane w AX, port w DX
}

static inline unsigned int inl(unsigned short port) {
  unsigned int result;
  __asm__ volatile("inl %1, %0" : "=a"(result) : "dN"(port));
  return result;
}

static inline void outl(unsigned short port, unsigned int data) {
  __asm__ volatile("outl %0, %1" : : "a"(data), "dN"(port));
}

uint8_t read_scancode() {
  // czekaj, aż PS/2 kontroler ma dane (bit 0 w porcie 0x64)
  while (!(inb(0x64) & 1))
    ;
  return inb(0x60);
}

char scancode_to_ascii(uint8_t scancode, bool shift) {
  static char map[128] = {0,    27,  '1', '2',  '3',  '4',  '5', '6', '7',  '8',
                          '9',  '0', '-', '=',  '\b', '\t', 'q', 'w', 'e',  'r',
                          't',  'y', 'u', 'i',  'o',  'p',  '[', ']', '\n', 0,
                          'a',  's', 'd', 'f',  'g',  'h',  'j', 'k', 'l',  ';',
                          '\'', '`', 0,   '\\', 'z',  'x',  'c', 'v', 'b',  'n',
                          'm',  ',', '.', '/',  0,    '*',  0,   ' '};
  static char mapShift[128] = {
      0,   27,  '!',  '@',  '#',  '$', '%', '^', '&', '*', '(', ')',
      '_', '+', '\b', '\t', 'Q',  'W', 'E', 'R', 'T', 'Y', 'U', 'I',
      'O', 'P', '{',  '}',  '\n', 0,   'A', 'S', 'D', 'F', 'G', 'H',
      'J', 'K', 'L',  ':',  '"',  '~', 0,   '|', 'Z', 'X', 'C', 'V',
      'B', 'N', 'M',  '<',  '>',  '?', 0,   '*', 0,   ' '};

  if (scancode > 127)
    return 0;
  if (shift)
    return mapShift[scancode];
  return map[scancode];
}

void poweroff() {
    // QEMU/VMware/Bochs
    outw(0x604, 0x2000);   // main method for QEMU and VMware
    outb(0xF4, 0x00);      // fallback for older QEMU
    outw(0xB004, 0x2000);  // fallback for Bochs/APM BIOS

    // if all else fails — stop CPU
    asm volatile("cli; hlt");
}