#include "debug.c"
#include "disk.c"
#include "helpers.c"
#include "split.c"
#include "term.c"
#include <stdint.h>

void kernel_main(void) {
  terminal_initialize();
  // terminal_writestring("Hello, kernel World!\r\n");
  DebugWriteString("Hello, world! From E9.\r\n");

  terminal_writestring("Welcome to NickOS! Please login as live user "
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
    char line[1024];

    terminal_writestring("> ");

    input_line(line, sizeof(line));

    char *fragments[1024];
    int fragmentCount = split(line, ' ', fragments, 256);

    if (fragmentCount > 0) {
      const char *cmd = fragments[0];
      if (strcmp(cmd, "echo") == 0) {
        for (size_t i = 1; i < fragmentCount; i++) {
          terminal_writestring(fragments[i]);
          if (i == fragmentCount - 1)
            terminal_writestring("\n");
          else
            terminal_writestring(" ");
        }
      } else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0) {
        terminal_clear();
      } else if (strcmp(cmd, "help") == 0) {
        terminal_writestring(
            "All commands in NickOS:\nhelp - shows all commands\nclear - "
            "clears terminal\ncls - alias for clear\necho <text> - prints text "
            "given in <text> argument\n");
      } else if (strcmp(cmd, "dbgdisk") == 0) {
        uint16_t sector[256];

        ata_read_sector(0, sector);

        for (size_t i = 0; i < 256; i++) {
          sector[i] = swap_endian16(sector[i]);
        }

        uint8_t *sectorBytes = (uint8_t *)sector;

        DebugWriteString((char *)sectorBytes);
      }
      // else{
      //   terminal_writestring("Command not found!\n");
      // }
    }
  }
}
