#define _BITS_FLOATN_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "cdrom.c"
#include "debug.c"
#include "disk.c"
#include "split.c"
#include "term.c"
#include "utils.c"
#include "iso9660.c"
#include "fat32.c"
#include "apps/nickfetch.c"

bool logged;

void kernel_main(void)
{
  terminal_initialize();
  // terminal_writestring("Hello, kernel World!\r\n");

  DebugWriteString("Hello, world! From E9.\r\n");

  fat32_init(0);

  terminal_writestring_format(
      "Welcome to $9Nick$4OS $70.0.0 build 2!\nPlease login as $1live user "
      "$7(\"$Bliveuser$7\"). Password is \"$31234$7\".\n\n");

  while (true)
  {
    if (logged)
    {
      char line[4096];

      terminal_writestring("> ");

      input_line(line, sizeof(line));

      char *fragments[4096];
      int fragmentCount = split(line, ' ', fragments, 256);

      if (fragmentCount > 0)
      {
        const char *cmd = fragments[0];
        if (strcmp(cmd, "echo") == 0)
        {
          char text[4096];

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
              fat32_write_file_by_path(parts[1], parts[0], strlen(parts[0]), FAT32_WRITE_OVERWRITE);
            }
            else if (frags == 3)
            {
              DebugWriteString("appending file ");
              DebugWriteString(parts[2]);
              DebugWriteString(" with: ");
              DebugWriteString(parts[0]);
              DebugWriteString("\n");
              fat32_write_file_by_path(parts[2], parts[0], strlen(parts[0]), FAT32_WRITE_APPEND);
            }
          }

          terminal_writestring(text);
          terminal_writestring("\n");
        }
        else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0)
        {
          terminal_clear();
        }
        else if (strcmp(cmd, "diskinfo") == 0)
        {
          terminal_writestring("DISK:\n");
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
          // terminal_writestring("CD-ROM:\n");
          // char cdsize[32];

          // itoa_bare(cdsize, 32, atapi_get_disc_size(), 10);
          // terminal_writestring(cdsize);
          // terminal_writestring(" sectors\n");
        }
        // else if (strcmp(cmd, "lscd") == 0)
        // {
        //   if (fragmentCount <= 1)
        //     iso_list_by_path("/");
        //   else
        //     iso_list_by_path(fragments[1]);
        // }
        else if (strcmp(cmd, "cat") == 0)
        {
          if (fragmentCount == 1)
          {
          }
          else
            cmd_cat(fragments[1]);
        }
        else if (strcmp(cmd, "ls") == 0)
        {
          if (fragmentCount <= 1)
          {
            terminal_writestring("/home\n");
            terminal_writestring("/cdrom\n");
          }
          else
          {
            if (memcmp(fragments[1], "/cdrom/", 7) == 0 || memcmp(fragments[1], "/cdrom ", 7) == 0 || memcmp(fragments[1], "/cdrom\0", 7) == 0)
            {
              iso_list_by_path(fragments[1] + 6);
            }
            else if (memcmp(fragments[1], "/home/", 7) == 0 || memcmp(fragments[1], "/home ", 7) == 0 || memcmp(fragments[1], "/home\0", 6) == 0)
            {
              fat32_ls_path(fragments[1] + 5);
            }
          }
          fat32_ls_path(fragments[1]);
        }
        else if (strcmp(cmd, "poweroff") == 0 ||
                 strcmp(cmd, "shutdown") == 0)
        {
          poweroff();
        }
        else if (strcmp(cmd, "reboot") == 0 || strcmp(cmd, "restart") == 0)
        {
          outb(0x64, 0xFE);
        }
        else if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "logout") == 0)
        {
          logged = false;
          terminal_clear();
          terminal_writestring(
              "Welcome to $9Nick$4OS $70.0.0 build 2!\nPlease login as $1live user "
              "$7(\"$Bliveuser$7\"). Password is \"$31234$7\".\n\n");
        }
        else if (strcmp(cmd, "help") == 0)
        {
          terminal_writestring(
              "All commands in NickOS:\nhelp - shows all commands\nclear - "
              "clears terminal\ncls - alias for clear\necho <text> - prints "
              "text "
              "given in <text> argument\ndiskinfo - shows informations about "
              "ATA "
              "drive\nreboot - restarts a system\nrestart - alias for "
              "reboot\npoweroff - shutdowns a system\nshutdown - alias for "
              "poweroff\nexit - logs out from system\nlogout - alias for "
              "exit\nlscd [path] - list files in CD-ROM. Default path is /\nls [path] - list files on HDD (FAT32). Default path is /\ncat <path> - read file content and display");
        }
        else if (strcmp(cmd, "nickfetch") == 0)
        {
          execute_nickfetch();
        }
        else
        {
          terminal_writestring("Command not found!\n");
        }
      }
    }
    else
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
      logged = true;
    }
  }
}
