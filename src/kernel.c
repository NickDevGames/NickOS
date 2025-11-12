#include "debug.c"
#include "disk.c"
#include "helpers.c"
#include "split.c"
#include "term.c"
#include <stdint.h>

void kernel_main(void)
{
  terminal_initialize();
  // terminal_writestring("Hello, kernel World!\r\n");
  DebugWriteString("Hello, world! From E9.\r\n");

  terminal_writestring("Welcome to NickOS! Please login as live user "
                       "(\"liveuser\"). Password is \"1234\".\n\n");

  while (true)
  {
    terminal_writestring("NickOS login: ");
    char login[256];
    input_line(login, sizeof(login));

    terminal_writestring("NickOS password: ");
    char password[256];
    input_line_pass(password, sizeof(password));

    if (strcmp(login, "liveuser"))
    {
      terminal_writestring("Wrong login!\n");
      continue;
    }
    if (strcmp(password, "1234"))
    {
      terminal_writestring("Wrong password!\n");
      continue;
    }

    terminal_writestring(
        "Logged in! You can run \"help\" command for commands\n");
    break;
  }

  while (true)
  {
    char line[1024];

    terminal_writestring("> ");

    input_line(line, sizeof(line));

    char *fragments[1024];
    int fragmentCount = split(line, ' ', fragments, 256);

    if (fragmentCount > 0)
    {
      const char *cmd = fragments[0];
      if (strcmp(cmd, "echo") == 0)
      {
        char text[1024];

        join_args(fragments, fragmentCount, text, sizeof(text));

        char *parts[3];
        size_t frags = split(text, '>', parts, 3);
        if (frags > 1)
        {
          if (frags == 2)
          {
            DebugWriteString("overriding file ");
            DebugWriteString(parts[1]);
            DebugWriteString(" with: ");
            DebugWriteString(parts[0]);
            DebugWriteString("\n");
          }
          else if (frags == 3)
          {
            DebugWriteString("appending file ");
            DebugWriteString(parts[2]);
            DebugWriteString(" with: ");
            DebugWriteString(parts[0]);
            DebugWriteString("\n");
          }
        }

        terminal_writestring(text);
        terminal_writestring("\n");
      }
      else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0)
      {
        terminal_clear();
      }
      else if (strcmp(cmd, "help") == 0)
      {
        terminal_writestring(
            "All commands in NickOS:\nhelp - shows all commands\nclear - "
            "clears terminal\ncls - alias for clear\necho <text> - prints text "
            "given in <text> argument\n");
      }
      else if (strcmp(cmd, "diskinfo") == 0)
      {
        ata_identify_t ataid;

        ata_identify(&ataid);

        char buf[32];

        itoa_bare(buf, sizeof(buf), ataid.sectors, 10);
      
        terminal_writestring("Number of sectors: ");
        terminal_writestring(buf);
        terminal_writestring("\n");

        terminal_writestring("Disk model: ");
        terminal_writestring(ataid.model);
        terminal_writestring("\n");
      }
      else
      {
        terminal_writestring("Command not found!\n");
      }
    }
  }
}
