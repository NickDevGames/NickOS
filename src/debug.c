#pragma once

#include "helpers.c"
#include "io.c"
#include "string.c"

static inline void DebugWriteCharacter(char chr) { outb(0xE9, chr); }

static inline void DebugWriteString(const char *str) {
  for (size_t i = 0; i < strlen(str); ++i)
    DebugWriteCharacter(str[i]);
}

void debug_pointer(char *ptr) {
  char buffer[20];

  // Rzutujemy wskaźnik na liczbę całkowitą
  uintptr_t addr = (uintptr_t)ptr;

  // Konwertujemy adres na string w systemie szesnastkowym
  itoa_bare(buffer, 20, (int)addr, 16);

  DebugWriteString(buffer);
}