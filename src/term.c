#pragma once

#include "io.c"
#include "string.c"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define VGA_WIDTH 80
#define VGA_HEIGHT 25

struct Point {
  size_t X, Y;
};

enum TextModeColors {
  VGA_COLOR_BLACK = 0,
  VGA_COLOR_BLUE = 1,
  VGA_COLOR_GREEN = 2,
  VGA_COLOR_CYAN = 3,
  VGA_COLOR_RED = 4,
  VGA_COLOR_MAGENTA = 5,
  VGA_COLOR_BROWN = 6,
  VGA_COLOR_LIGHT_GREY = 7,
  VGA_COLOR_DARK_GREY = 8,
  VGA_COLOR_LIGHT_BLUE = 9,
  VGA_COLOR_LIGHT_GREEN = 10,
  VGA_COLOR_LIGHT_CYAN = 11,
  VGA_COLOR_LIGHT_RED = 12,
  VGA_COLOR_LIGHT_MAGENTA = 13,
  VGA_COLOR_LIGHT_BROWN = 14,
  VGA_COLOR_WHITE = 15,
};

struct Point cursorPos;
uint8_t terminal_color;
uint16_t *terminal_buffer;

static const inline uint8_t vga_entry_color(enum TextModeColors fg,
                                            enum TextModeColors bg) {
  return fg | bg << 4;
}

static const inline uint16_t vga_entry(unsigned char uc, uint8_t color) {
  return (uint16_t)uc | (uint16_t)color << 8;
}

void terminal_clear(void) {
  for (size_t i = 0; i < VGA_HEIGHT * VGA_WIDTH; ++i)
    terminal_buffer[i] = vga_entry(' ', terminal_color);
  cursorPos.X = 0;
  cursorPos.Y = 0;
}

void terminal_initialize(void) {
  terminal_color = vga_entry_color(VGA_COLOR_LIGHT_GREY, VGA_COLOR_BLACK);
  terminal_buffer = (uint16_t *)0xB8000;
  terminal_clear();
}

static inline void terminal_setcolor(uint8_t color) { terminal_color = color; }

static inline void terminal_putentryat(char c, uint8_t color,
                                       struct Point cursorPos) {
  terminal_buffer[cursorPos.Y * VGA_WIDTH + cursorPos.X] = vga_entry(c, color);
}

void terminal_putchar(char c) {
  if (c == '\n') {
    cursorPos.Y++;
    cursorPos.X = 0;
  } else if (c == '\r') {
    cursorPos.X = 0;
  } else if (c == '\b') {
    cursorPos.X--;
    terminal_putentryat(' ', terminal_color, cursorPos);
  } else {
    terminal_putentryat(c, terminal_color, cursorPos);
    if (++cursorPos.X >= VGA_WIDTH) {
      cursorPos.X = 0;
      ++cursorPos.Y;
    }
    if (cursorPos.Y >= VGA_HEIGHT)
      cursorPos.Y = 0;
  }
}

void terminal_refresh_cursor() {
  outb(0x3D4, 0x0F);
  outb(0x3D5, (uint8_t)((cursorPos.Y * VGA_WIDTH + cursorPos.X) & 0xFF));
  outb(0x3D4, 0x0E);
  outb(0x3D5, (uint8_t)(((cursorPos.Y * VGA_WIDTH + cursorPos.X) >> 8) & 0xFF));
}

void terminal_write(const char *data, size_t size) {
  for (size_t i = 0; i < size; i++)
    terminal_putchar(data[i]);
  terminal_refresh_cursor();
}

void terminal_writestring(const char *data) {
  terminal_write(data, strlen(data));
}

void terminal_write_format(const char *data, size_t size) {
  static enum TextModeColors fg = VGA_COLOR_LIGHT_GREY;
  static enum TextModeColors bg = VGA_COLOR_BLACK;

  // Ustaw domyślny kolor na start
  terminal_color = vga_entry_color(fg, bg);

  for (size_t i = 0; i < size; i++) {
    char c = data[i];

    if ((c == '$' || c == '&') && i + 1 < size) {
      char next = data[++i];

      // Zamień hex (0–9, A–F, a–f) na liczbę
      uint8_t color;
      if (next >= '0' && next <= '9')
        color = next - '0';
      else if (next >= 'A' && next <= 'F')
        color = next - 'A' + 10;
      else if (next >= 'a' && next <= 'f')
        color = next - 'a' + 10;
      else
        continue; // nieprawidłowy znak — pomiń

      if (c == '$')
        fg = color;
      else
        bg = color;

      terminal_color = vga_entry_color(fg, bg);
      continue;
    }

    // <-- Kluczowy punkt:
    // Funkcja terminal_putchar MUSI używać terminal_color
    terminal_putchar(c);
  }

  terminal_refresh_cursor();
}

void terminal_writestring_format(const char *data) {
  terminal_write_format(data, strlen(data));
}

bool shift = false;

void input_line(char *buffer, int max_len) {
  int i = 0;

  while (i < max_len - 1) {
    uint8_t sc = read_scancode();

    if (sc == 0x2A || sc == 0x36) {
      shift = true;
      continue;
    }

    if (sc == 0xAA || sc == 0xB6) {
      shift = false;
      continue;
    }
    // ignoruj klawisze puszczane (bit 7 ustawiony)
    if (sc & 0x80)
      continue;

    char c = scancode_to_ascii(sc, shift);
    if (!c)
      continue;

    if (c == '\n') {
      terminal_putchar('\n');
      break;
    } else if (c == '\b') {
      if (i > 0) {
        i--;
        terminal_putchar('\b'); // możesz chcieć nadpisać spacją i cofnąć kursor
      }
    } else {
      buffer[i++] = c;
      terminal_putchar(c);
    }

    terminal_refresh_cursor();
  }

  buffer[i] = '\0';
}

void input_line_pass(char *buffer, int max_len) {
  int i = 0;

  while (i < max_len - 1) {
    uint8_t sc = read_scancode();

    if (sc == 0x2A || sc == 0x36) {
      shift = true;
      continue;
    }

    if (sc == 0xAA || sc == 0xB6) {
      shift = false;
      continue;
    }

    // ignoruj klawisze puszczane (bit 7 ustawiony)
    if (sc & 0x80)
      continue;

    char c = scancode_to_ascii(sc, shift);
    if (!c)
      continue;

    if (c == '\n') {
      terminal_putchar('\n');
      break;
    } else if (c == '\b') {
      if (i > 0) {
        i--;
        terminal_putchar('\b'); // możesz chcieć nadpisać spacją i cofnąć kursor
      }
    } else {
      buffer[i++] = c;
      terminal_putchar('*');
    }

    terminal_refresh_cursor();
  }

  buffer[i] = '\0';
}