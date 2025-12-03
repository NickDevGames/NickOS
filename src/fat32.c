#include <string.h>
#include <stdint.h>

#pragma pack(push,1)
typedef struct {
    uint8_t  jump[3];
    char     oem[8];
    uint16_t bytes_per_sector;
    uint8_t  sectors_per_cluster;
    uint16_t reserved_sectors;
    uint8_t  fat_count;
    uint16_t root_entry_count;     // 0 dla FAT32
    uint16_t total_sectors_16;
    uint8_t  media_type;
    uint16_t sectors_per_fat_16;
    uint16_t sectors_per_track;
    uint16_t number_of_heads;
    uint32_t hidden_sectors;
    uint32_t total_sectors_32;

    uint32_t sectors_per_fat_32;   // ważne dla FAT32
    uint16_t ext_flags;
    uint16_t fs_version;
    uint32_t root_cluster;         // Katalog główny
    uint16_t fs_info;
    uint16_t backup_boot_sector;
    uint8_t  reserved[12];
    uint8_t  drive_number;
    uint8_t  reserved2;
    uint8_t  boot_signature;
    uint32_t volume_id;
    char     volume_label[11];
    char     fs_type[8];
} fat32_bpb_t;

typedef struct {
    char     name[11];      // 8.3
    uint8_t  attr;
    uint8_t  nt_reserved;
    uint8_t  creation_time_tenth;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t access_date;
    uint16_t cluster_high;
    uint16_t write_time;
    uint16_t write_date;
    uint16_t cluster_low;
    uint32_t file_size;
} fat32_directory_entry_t;
#pragma pack(pop)

typedef struct {
    uint32_t first_cluster;
    uint32_t size;
    int is_directory;
} fat32_dir_entry_info_t;


fat32_bpb_t fat32_bpb;

uint32_t fat_begin_lba;
uint32_t cluster_begin_lba;

uint32_t bytes_per_sector;
uint32_t sectors_per_cluster;

uint32_t fat_start_lba;

static void memcpy_c(void *dst, const void *src, int n) {
    char *d = (char*)dst; const char *s = (const char*)src;
    while(n--) *d++ = *s++;
}

void fat32_init(uint32_t lba) {
    fat_start_lba = lba;
    ata_read_sector(lba, (uint16_t*)&fat32_bpb);

    bytes_per_sector     = fat32_bpb.bytes_per_sector;
    sectors_per_cluster  = fat32_bpb.sectors_per_cluster;

    fat_begin_lba        = lba + fat32_bpb.reserved_sectors;
    cluster_begin_lba    = fat_begin_lba + fat32_bpb.fat_count * fat32_bpb.sectors_per_fat_32;
}

void fat32_read_cluster(uint32_t cluster, uint8_t *buffer) {
    uint32_t first_sector = cluster_begin_lba + (cluster - 2) * sectors_per_cluster;

    for (uint32_t i = 0; i < sectors_per_cluster; i++) {
        ata_read_sector(first_sector + i, (uint16_t*)(buffer + i * bytes_per_sector));
    }
}

void fat32_list_directory(uint32_t cluster) {
    uint8_t cluster_buf[4096]; // wystarczy dla 32 KB klastra

    fat32_read_cluster(cluster, cluster_buf);

    fat32_directory_entry_t *ent = (fat32_directory_entry_t*)cluster_buf;

    for (int i = 0; i < (4096 / sizeof(fat32_directory_entry_t)); i++) {
        if (ent[i].name[0] == 0x00) break;
        if (ent[i].name[0] == 0xE5) continue;
        if (ent[i].attr == 0x0F) continue;

        char name[12];
        memcpy_c(name, ent[i].name, 11);
        name[11] = 0;

        int is_dir = ent[i].attr & 0x10;

        terminal_writestring(name);
        if (is_dir) terminal_writestring("/");
        terminal_writestring("\n");
    }
}

uint32_t fat32_find_in_directory(uint32_t cluster, const char *name_8_3);

uint32_t fat32_get_cluster_of_path(const char *path) {
    uint32_t cluster = fat32_bpb.root_cluster;

    if (path[0] == '/') path++;

    char part[64];
    int pos = 0;

    while (*path) {
        if (*path == '/') {
            part[pos] = 0;
            cluster = fat32_find_in_directory(cluster, part);
            char buf[32];
            itoa_bare(buf, 32, cluster, 10);
            DebugWriteString(buf);
            pos = 0;
            path++;
            continue;
        }
        part[pos++] = *path++;
    }

    if (pos > 0) {
        part[pos] = 0;
        cluster = fat32_find_in_directory(cluster, part);
    }

    return cluster;
}

void fat32_ls_path(const char *path) {
    uint32_t cluster = fat32_get_cluster_of_path(path);
    fat32_list_directory(cluster);
}

// -----------------------------
// Helper: simple toupper
// -----------------------------
static char upcase(char c) {
    if (c >= 'a' && c <= 'z') return c - 32;
    return c;
}

// -----------------------------
// make_short_name_from_input
// Convert a user filename (like "readme.txt" or "DIR") into
// an 11-byte 8.3 buffer (padded with spaces, uppercased).
// This is a simple converter: no LFN support, ignores weird chars.
// out_short must be at least 11 bytes.
// -----------------------------
void make_short_name_from_input(const char *input, uint8_t out_short[11]) {
    int i;
    // fill with spaces
    for (i = 0; i < 11; i++) out_short[i] = ' ';

    // find dot (if any)
    const char *dot = 0;
    const char *p = input;
    while (*p) { if (*p == '.') { dot = p; break; } p++; }

    // copy name part (up to 8)
    int j = 0;
    p = input;
    while (*p && *p != '.' && j < 8) {
        // map to upper and store
        out_short[j++] = (uint8_t)upcase(*p);
        p++;
    }

    // copy extension (if present) up to 3 chars
    if (dot) {
        p = dot + 1;
        int k = 0;
        while (*p && k < 3) {
            out_short[8 + k] = (uint8_t)upcase(*p);
            k++; p++;
        }
    }
}

// -----------------------------
// compare_short_name
// Compare 11-byte directory name with generated short name.
// Returns 1 if equal, 0 otherwise.
// -----------------------------
int compare_short_name(const uint8_t entry_name[11], const uint8_t short_name[11]) {
    for (int i = 0; i < 11; i++) {
        if (entry_name[i] != short_name[i]) return 0;
    }
    return 1;
}

// Returns next cluster in chain.
// If returns >= 0x0FFFFFF8 → end of chain.
// If returns 0 → error or free cluster.
uint32_t fat32_next_cluster(uint32_t cluster)
{
    // FAT32 masks some high bits; only 28 bits are valid
    cluster &= 0x0FFFFFFF;

    // Each FAT entry is 4 bytes
    uint32_t fat_offset = cluster * 4;

    // Which sector of FAT?
    uint32_t fat_sector = fat_start_lba + (fat_offset / bytes_per_sector);

    // Position inside sector
    uint32_t offset_in_sector = fat_offset % bytes_per_sector;

    // Read FAT sector
    static uint8_t fat_sector_buf[512];
    ata_read_sector(fat_sector, (uint16_t*)fat_sector_buf);

    // Read 32-bit entry
    uint32_t value = *(uint32_t*)(fat_sector_buf + offset_in_sector);

    // Mask to 28 bits (FAT32 spec)
    value &= 0x0FFFFFFF;

    return value;
}


// -----------------------------
// fat32_find_in_directory
// Search a directory (given by cluster) for an entry named 'name' (user input).
// Returns cluster number of the found entry (first cluster of file/dir), or 0 if not found.
// - Skips deleted entries and LFN entries.
// - Stops at 0x00 (end of directory).
// - Does NOT support matching long names (LFN).
// -----------------------------
uint32_t fat32_find_in_directory(uint32_t dirCluster, const char *name) {
    if (dirCluster < 2) return 0;

    // prepare short-name to match (11 bytes)
    uint8_t want[11];
    make_short_name_from_input(name, want);

    // allocate a buffer sized to cluster (stack or static)
    // be conservative: cluster size up to, e.g., 32 sectors * 512 = 16384 bytes
    // If sectors_per_cluster * bytes_per_sector is large, you might want static buffer.
    uint32_t cluster_size = sectors_per_cluster * bytes_per_sector;
    // For simplicity use a static buffer (avoid stack overflow):
    static uint8_t cluster_buf[16384]; // supports up to 32 * 512 = 16384; adjust if needed
    if (cluster_size > sizeof(cluster_buf)) {
        // cluster too large for buffer — fail gracefully
        return 0;
    }

    uint32_t cluster = dirCluster;

    while (cluster >= 2 && cluster < 0x0FFFFFF8) {
        // read full cluster
        fat32_read_cluster(cluster, cluster_buf);

        // iterate entries: 32 bytes each
        uint32_t entries = cluster_size / 32;
        for (uint32_t i = 0; i < entries; i++) {
            uint8_t *entry = cluster_buf + i * 32;

            uint8_t first = entry[0];

            if (first == 0x00) {
                // end of directory entries
                return 0;
            }
            if (first == 0xE5) {
                // deleted entry
                continue;
            }

            uint8_t attr = entry[11];
            if ((attr & 0x0F) == 0x0F) {
                // long file name entry -> skip
                continue;
            }

            // now compare 11-byte name
            if (compare_short_name(entry, want)) {
                // found — read cluster high/low
                uint16_t low = *(uint16_t*)(entry + 26);   // first_cluster_low (offset 26)
                uint16_t high = *(uint16_t*)(entry + 20);  // first_cluster_high (offset 20)
                uint32_t found_cluster = ((uint32_t)high << 16) | (uint32_t)low;
                return found_cluster;
            }
        }

        // go to next cluster in directory chain
        cluster = fat32_next_cluster(cluster);
        if (cluster == 0 || cluster >= 0x0FFFFFF8) break;
    }

    return 0; // not found
}

// Reads file content and prints it via terminal_writestring.
// Assumes:
// - "cluster" is the first cluster of the file (from directory entry)
// - "size" is the exact file size in bytes.
// Prints exactly file content (no null-termination).
void fat32_read_file(uint32_t first_cluster, uint32_t size)
{
    if (first_cluster < 2) {
        terminal_writestring("Invalid file cluster!\n");
        return;
    }

    uint32_t cluster = first_cluster;

    // cluster size in bytes
    uint32_t cluster_size = sectors_per_cluster * bytes_per_sector;

    // static buffer for one cluster (max 16 KB; adjust if bigger clusters)
    static uint8_t cluster_buf[16384];
    if (cluster_size > sizeof(cluster_buf)) {
        terminal_writestring("Cluster size too big for buffer!\n");
        return;
    }

    uint32_t bytes_left = size;

    while (cluster >= 2 && cluster < 0x0FFFFFF8 && bytes_left > 0)
    {
        // Read this cluster
        fat32_read_cluster(cluster, cluster_buf);

        // How many bytes to print from this cluster?
        uint32_t to_print = (bytes_left < cluster_size) ? bytes_left : cluster_size;

        // Temporary null-termination for terminal_writestring
        // (Assumes terminal_writestring expects a C-string)
        cluster_buf[to_print] = 0;

        terminal_writestring((char*)cluster_buf);

        bytes_left -= to_print;

        // move to next cluster
        if (bytes_left > 0)
            cluster = fat32_next_cluster(cluster);
        else
            break;
    }
}

// Returns 1 on success, 0 on failure (path not found).
// On success fills info with cluster, size, is_directory.
// Works only on absolute paths starting with '/'.
// Does NOT support '.' or '..' or LFN.

int fat32_resolve_path(const char *path, fat32_dir_entry_info_t *info) {
    if (!info) return 0;

    uint32_t cluster = fat32_bpb.root_cluster;

    // Skip leading '/'
    if (*path == '/')
        path++;

    char part[13];  // 8.3 max length + null
    int pos = 0;

    // If empty path → root directory
    if (*path == 0) {
        info->first_cluster = cluster;
        info->size = 0;
        info->is_directory = 1;
        return 1;
    }

    while (*path) {
        pos = 0;

        // Extract next path component (up to '/')
        while (*path && *path != '/') {
            if (pos < 12)
                part[pos++] = *path;
            path++;
        }
        part[pos] = 0;

        if (*path == '/')
            path++;

        // Find entry in current directory cluster
        uint8_t cluster_buf[16384]; // max cluster buffer; adjust if needed
        uint32_t cluster_size = sectors_per_cluster * bytes_per_sector;

        uint32_t current_cluster = cluster;

        int found = 0;

        while (current_cluster >= 2 && current_cluster < 0x0FFFFFF8) {
            fat32_read_cluster(current_cluster, cluster_buf);

            uint32_t entries = cluster_size / 32;

            for (uint32_t i = 0; i < entries; i++) {
                uint8_t *entry = cluster_buf + i * 32;

                uint8_t first = entry[0];
                if (first == 0x00) {
                    // End of directory
                    break;
                }
                if (first == 0xE5) continue;        // deleted
                if ((entry[11] & 0x0F) == 0x0F) continue; // LFN skip

                uint8_t want[11];
                make_short_name_from_input(part, want);

                if (compare_short_name(entry, want)) {
                    // Found
                    uint16_t low = *(uint16_t*)(entry + 26);
                    uint16_t high = *(uint16_t*)(entry + 20);
                    uint32_t first_cluster = ((uint32_t)high << 16) | low;

                    uint32_t size = *(uint32_t*)(entry + 28);
                    uint8_t attr = entry[11];
                    int is_dir = (attr & 0x10) != 0;

                    // If last component, fill info and return success
                    if (*path == 0) {
                        info->first_cluster = first_cluster;
                        info->size = size;
                        info->is_directory = is_dir;
                        return 1;
                    } else {
                        // Descend into directory
                        if (!is_dir) {
                            // path continues but this is file → fail
                            return 0;
                        }
                        cluster = first_cluster;
                        found = 1;
                        break;
                    }
                }
            }

            if (found) break;

            current_cluster = fat32_next_cluster(current_cluster);
        }

        if (!found) return 0;  // Not found
    }

    return 0; // Should not reach here
}


void cmd_cat(const char *path)
{
    fat32_dir_entry_info_t info;
    if (!fat32_resolve_path(path, &info)) {
        terminal_writestring("File not found.\n");
        return;
    }

    if (info.is_directory) {
        terminal_writestring("Cannot cat a directory.\n");
        return;
    }

    fat32_read_file(info.first_cluster, info.size);
}
