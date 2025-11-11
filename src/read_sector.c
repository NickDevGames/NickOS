extern unsigned short cylinder;
extern unsigned char head;
extern unsigned char sector;
extern unsigned char disk;
extern unsigned short buffer_seg;
extern unsigned short buffer_off;

extern void read_sector_stub();

void read_sector_c(unsigned short cyl, unsigned char hd, unsigned char sec, unsigned char dk, void* buffer) {
    unsigned int buf_addr = (unsigned int)buffer;
    cylinder = cyl;
    head = hd;
    sector = sec;
    disk = dk;

    buffer_seg = buf_addr >> 4;
    buffer_off = buf_addr & 0xF;

    read_sector_stub();
}
