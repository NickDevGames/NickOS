#include "cdrom.c"
#include "debug.c"
#include "disk.c"
#include "split.c"
#include "term.c"
#include "utils.c"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void kernel_main(void) {
  terminal_initialize();
  // terminal_writestring("Hello, kernel World!\r\n");
  DebugWriteString("Hello, world! From E9.\r\n");

  terminal_writestring(
      "Welcome to NickOS 0.0.0 build 1!\nPlease login as live user "
      "(\"liveuser\"). Password is \"1234\".\n\n");

  while (true) {
    terminal_writestring("NickOS login: ");
    char login[256];
    input_line(login, sizeof(login));

    terminal_writestring("NickOS password: ");
    char password[256];
    input_line_pass(password, sizeof(password));

    if (strcmp(login, "liveuser")) {
      terminal_writestring("Wrong login!\n");
      continue;
    }
    if (strcmp(password, "1234")) {
      terminal_writestring("Wrong password!\n");
      continue;
    }

    terminal_writestring(
        "Logged in! You can run \"help\" command for commands\n");
    break;
  }

  while (true) {
    char line[4096];

    terminal_writestring("> ");

    input_line(line, sizeof(line));

    char *fragments[4096];
    int fragmentCount = split(line, ' ', fragments, 256);

    if (fragmentCount > 0) {
      const char *cmd = fragments[0];
      if (strcmp(cmd, "echo") == 0) {
        char text[4096];

        join_args(fragments, fragmentCount, text, sizeof(text));

        char *parts[3];
        size_t frags = split(text, '>', parts, 3);
        if (frags > 1) {
          if (frags == 2) {
            DebugWriteString("overriding file ");
            DebugWriteString(parts[1]);
            DebugWriteString(" with: ");
            DebugWriteString(parts[0]);
            DebugWriteString("\n");
          } else if (frags == 3) {
            DebugWriteString("appending file ");
            DebugWriteString(parts[2]);
            DebugWriteString(" with: ");
            DebugWriteString(parts[0]);
            DebugWriteString("\n");
          }
        }

        terminal_writestring(text);
        terminal_writestring("\n");
      } else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0) {
        terminal_clear();
      } else if (strcmp(cmd, "help") == 0) {
        terminal_writestring(
            "All commands in NickOS:\nhelp - shows all commands\nclear - "
            "clears terminal\ncls - alias for clear\necho <text> - prints text "
            "given in <text> argument\ndiskinfo - shows informations about ATA "
            "drive\n");
      } else if (strcmp(cmd, "diskinfo") == 0) {
        ata_identify_t ataid;

        ata_identify(&ataid);

        char nos[32];
        itoa_bare(nos, sizeof(nos), ataid.sectors, 10);

        char lss[32];
        itoa_bare(lss, sizeof(lss), ataid.logical_sector_size, 10);

        char pss[32];
        itoa_bare(pss, sizeof(pss), ataid.sectors, 10);

        terminal_writestring("Number of sectors: ");
        terminal_writestring(nos);
        terminal_writestring("\n");

        terminal_writestring("Disk model: ");
        terminal_writestring(ataid.model);
        terminal_writestring("\n");

        terminal_writestring("Logical sector size in bytes: ");
        terminal_writestring(lss);
        terminal_writestring("\n");

        terminal_writestring("Physical sector size in bytes: ");
        terminal_writestring(pss);
        terminal_writestring("\n");
      } else if (strcmp(cmd, "poweroff") == 0 || strcmp(cmd, "shutdown") == 0) {
        poweroff();
      } else if (strcmp(cmd, "install") == 0) {
        terminal_writestring("Installing NickOS\n");
        uint16_t buffer[1024 * 1];
        
        read_cdrom(0x170, false, 0, 1, buffer);

        uint8_t *bufferBytes = (uint8_t *)buffer;
      } else {
        terminal_writestring("Command not found!\n");
      }
    }
  }
}
