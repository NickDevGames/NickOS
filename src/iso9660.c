#include <stdint.h>
#include "memory.c"

#define SECTOR_SIZE 2048

static uint16_t sector_buffer[SECTOR_SIZE / 2]; // 2048 bajtów

// Pomocnicze: wypis string zakończony \n
void print(const char *s)
{
    terminal_writestring(s);
    terminal_writestring("\n");
}

// Konwersja little-endian 4B (ISO9660 używa 32-bit LE/BE powtórzonego)
static uint32_t read32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16) | ((uint32_t)p[3] << 24);
}

// Czytaj sektor jako raw bajty
static uint8_t *read_sector_iso9660(uint32_t lba)
{
    atapi_read_sector(lba, sector_buffer);
    return (uint8_t *)sector_buffer;
}

static uint8_t names_equal(const char *a, const char *b, uint8_t b_len)
{
    for (uint8_t i = 0; i < b_len; i++)
    {
        if (a[i] == 0)
            return 0;
        if (a[i] != b[i])
            return 0;
    }
    return a[b_len] == 0;
}

// -----------------------------------------------
//      LISTOWANIE PLIKÓW ISO9660
// -----------------------------------------------
void list_cd_files()
{
    // --------------------------
    // 1. Wczytaj PVD z LBA 16
    // --------------------------
    uint8_t *pvd = read_sector_iso9660(16);

    // Sprawdzenie typu 1 = Primary Volume Descriptor
    if (pvd[0] != 1 || pvd[1] != 'C' || pvd[2] != 'D' || pvd[3] != '0' || pvd[4] != '0' || pvd[5] != '1')
    {
        print("Brak PVD!");
        return;
    }

    // Root Directory Record zaczyna się od offsetu 156 w PVD
    uint8_t *root = pvd + 156;

    uint32_t root_lba = read32(root + 2);   // Location of extent
    uint32_t root_size = read32(root + 10); // Size in bytes

    // Liczba sektorów katalogu
    uint32_t root_sectors = root_size / SECTOR_SIZE + 1;

    print("Lista plikow:");

    // -----------------------------------------
    // 2. Przechodzimy przez wszystkie sektory katalogu
    // -----------------------------------------
    for (uint32_t s = 0; s < root_sectors; s++)
    {

        uint8_t *dir = read_sector_iso9660(root_lba + s);
        uint32_t pos = 0;

        while (pos < SECTOR_SIZE)
        {

            uint8_t len = dir[pos];
            if (len == 0)
                break; // Koniec wpisów w sektorze

            uint8_t *rec = dir + pos;

            uint32_t extent = read32(rec + 2);
            uint32_t fsize = read32(rec + 10);
            uint8_t name_len = rec[32];
            const char *name = (const char *)(rec + 33);

            // Pomijamy "." i ".."
            if (!(name_len == 1 && (name[0] == 0 || name[0] == 1)))
            {

                // Tymczasowy bufor nazwy
                static char temp[128];
                uint32_t i;
                for (i = 0; i < name_len && i < 127; i++)
                    temp[i] = name[i];
                temp[i] = 0;

                terminal_writestring(temp);
                terminal_writestring("\n");
            }

            pos += len;
        }
    }
}

void iso_list_directory(uint32_t dir_lba, uint32_t dir_size)
{
    uint32_t sectors = dir_size / SECTOR_SIZE;
    if (dir_size % SECTOR_SIZE)
        sectors++;

    for (uint32_t s = 0; s < sectors; s++)
    {

        uint8_t *buf = read_sector_iso9660(dir_lba + s);
        uint32_t pos = 0;

        while (pos < SECTOR_SIZE)
        {

            uint8_t len = buf[pos];
            if (len == 0)
                break;

            uint8_t *rec = buf + pos;

            uint8_t flags = rec[25];
            uint8_t name_len = rec[32];
            const char *name = (const char *)(rec + 33);

            if (!(name_len == 1 && (name[0] == 0 || name[0] == 1)))
            {

                static char tmp[128];
                uint32_t i;

                for (i = 0; i < name_len && i < 127; i++)
                    tmp[i] = name[i];

                // Dodaj "/" jeśli katalog
                if (flags & 0x02 && i < 126)
                {
                    tmp[i++] = '/';
                }

                tmp[i] = 0;

                terminal_writestring(tmp);
                terminal_writestring("\n");
            }

            pos += len;
        }
    }
}

uint8_t iso_find_directory(
    uint32_t dir_lba,
    uint32_t dir_size,
    const char *name,
    uint32_t *out_lba,
    uint32_t *out_size)
{
    uint32_t sectors = dir_size / SECTOR_SIZE;
    if (dir_size % SECTOR_SIZE)
        sectors++;

    for (uint32_t s = 0; s < sectors; s++)
    {
        uint8_t *buf = read_sector_iso9660(dir_lba + s);
        uint32_t pos = 0;

        while (pos < SECTOR_SIZE)
        {

            uint8_t len = buf[pos];
            if (len == 0)
                break;

            uint8_t *rec = buf + pos;

            uint8_t flags = rec[25];
            uint8_t name_len = rec[32];
            const char *rec_name = (const char *)(rec + 33);

            // Pomijamy "." i ".."
            if (!(name_len == 1 && (rec_name[0] == 0 || rec_name[0] == 1)))
            {

                // porównanie
                if ((flags & 0x02) && names_equal(name, rec_name, name_len))
                {
                    *out_lba = read32(rec + 2);
                    *out_size = read32(rec + 10);
                    return 1;
                }
            }

            pos += len;
        }
    }
    return 0;
}

void iso_list_directory_by_name(
    uint32_t parent_lba,
    uint32_t parent_size,
    const char *dirname)
{
    uint32_t lba, size;

    if (!iso_find_directory(parent_lba, parent_size, dirname, &lba, &size))
    {
        terminal_writestring("Directory doesn't exist: ");
        terminal_writestring(dirname);
        terminal_writestring("\n");
        return;
    }

    // Wypisz zawartość
    terminal_writestring("Directory entries ");
    terminal_writestring(dirname);
    terminal_writestring(":\n");

    iso_list_directory(lba, size);
}

uint8_t iso_open_path(
    const char *path,
    uint32_t root_lba,
    uint32_t root_size,
    uint32_t *out_lba,
    uint32_t *out_size)
{
    // Jeśli ścieżka to "/", zwróć root
    if (path[0] == '/' && path[1] == 0)
    {
        *out_lba = root_lba;
        *out_size = root_size;
        return 1;
    }

    uint32_t cur_lba = root_lba;
    uint32_t cur_size = root_size;

    const char *p = path;

    // Skacz po '/'
    if (*p == '/')
        p++;

    static char part[128];
    uint8_t part_len = 0;

    for (;;)
    {

        // Zbierz jeden komponent ścieżki
        part_len = 0;
        while (*p != 0 && *p != '/')
        {
            part[part_len++] = *p++;
            if (part_len >= 127)
                break;
        }
        part[part_len] = 0;

        // Jeśli komponent nie jest pusty — schodzimy w katalog:
        if (part_len > 0)
        {
            uint32_t new_lba, new_size;

            if (!iso_find_directory(cur_lba, cur_size, part, &new_lba, &new_size))
            {
                return 0; // katalog nie istnieje
            }

            cur_lba = new_lba;
            cur_size = new_size;
        }

        // Koniec ścieżki?
        if (*p == 0)
            break;

        // Skip '/'
        if (*p == '/')
            p++;
    }

    *out_lba = cur_lba;
    *out_size = cur_size;

    return 1;
}

void iso_list_by_path(const char *path) {
    uint8_t *pvd = read_sector_iso9660(16);
    uint8_t *root = pvd + 156;

    uint32_t root_lba  = read32(root + 2);
    uint32_t root_size = read32(root + 10);

    uint32_t lba, size;
    if (!iso_open_path(path, root_lba, root_size, &lba, &size)) {
        terminal_writestring("Path doesn't exists: ");
        terminal_writestring(path);
        terminal_writestring("\n");
        return;
    }

    iso_list_directory(lba, size);
}

// Prototyp funkcji do odczytu pojedynczego sektora (2048 bajtów)
void atapi_read_sector(uint32_t lba, uint16_t *buffer);

uint8_t* atapi_read_file(uint32_t start_lba, uint32_t sectors_count)
{
    if (sectors_count == 0)
        return NULL;

    // Alokujemy bufor na całą zawartość pliku (2048 * sectors_count bajtów)
    uint8_t *file_data = (uint8_t*)malloc(sectors_count * 2048);
    if (!file_data)
        return NULL; // błąd alokacji

    // Bufor pośredni do odczytu sektora jako 16-bitowych słów
    uint16_t sector_buffer[1024];

    for (uint32_t i = 0; i < sectors_count; i++)
    {
        atapi_read_sector(start_lba + i, sector_buffer);

        // Kopiujemy sektor z sector_buffer (uint16_t) do file_data (uint8_t)
        // ATAPI zwraca dane w little endian 16-bit - a my chcemy ciąg bajtów
        for (int j = 0; j < 1024; j++)
        {
            // Little endian 16-bit: niska pozycja = młodszy bajt
            file_data[i * 2048 + j * 2]     = sector_buffer[j] & 0xFF;
            file_data[i * 2048 + j * 2 + 1] = (sector_buffer[j] >> 8) & 0xFF;
        }
    }

    return file_data;
}
