# NickOS

NickOS is a lightweight hobby operating system with its own terminal, filesystem handling, and a simple built-in command set. It is designed for learning, experimentation, and low-level development.

## Commands

- `help` – shows all commands
- `clear` – clears the terminal
- `cls` – alias for clear
- `echo <text>` – prints the provided text
- `diskinfo` – shows information about the ATA drive
- `reboot` – restarts the system
- `restart` – alias for reboot
- `poweroff` – powers off the system
- `shutdown` – alias for poweroff
- `exit` – logs out from the system
- `logout` – alias for exit
- `ls [path]` – lists files in the given path (default: /)
- `cat <path>` – displays the contents of a file

Note about shutdown:
The commands `poweroff` and `shutdown` work best in QEMU, where ACPI/APM is properly implemented. Other emulators or real machines may not fully power off.

## Creating a disk image for NickOS on Linux

NickOS uses a FAT32 disk image for storing files. The OS itself is loaded from the ISO, so nothing needs to be copied into the image unless you want to pre-create some files.

Create an empty image (example: 64 MB)
`dd if=/dev/zero of=disk.img bs=1M count=64`

Format it as FAT32
`sudo losetup -fP disk.img`
`sudo mkfs.fat -F32 /dev/loop0`
`sudo losetup -d /dev/loop0`

(Replace `/dev/loop0` if your loop device is different.)

Optional: create files inside the disk
`sudo losetup -fP disk.img`
`sudo mount /dev/loop0 /mnt`
`echo "Hello from Linux" | sudo tee /mnt/test.txt`
`sudo umount /mnt`
`sudo losetup -d /dev/loop0`

## Creating and formatting a disk image on Windows

Required tools:
• OSFMount
• FAT32 formatting tool (Windows format, fat32format, or WinImage)

Create empty disk image (64 MB)
`fsutil file createnew disk.img 67108864`

Mount the image with OSFMount
- Open OSFMount
- Select `Mount new...`
- Choose disk.img
- Set type to HDD
- Assign a drive letter

Format as FAT32
`format <drive_letter>: /FS:FAT32 /Q /V:NICKOS`

Optional: create files using Windows Explorer.

## Running NickOS in QEMU

Example command:

`qemu-system-i386 -cdrom NickOS.iso -boot d -debugcon file:/dev/stdout -m 256 -hda disk.img`

Explanation:
- `-cdrom build/NickOS.iso` loads the NickOS ISO
- `-boot d` makes QEMU boot from the CD-ROM
- `-debugcon file:/dev/stdout` sends debug output to stdout
- `-m 256` allocates 256 MB RAM
- `-hda disk.img` attaches your FAT32 disk image

You can adjust memory size, debug options, or additional drives as needed.

## TODO LIST
- [x] Input
- [x] Basic commands
- [x] Implement raw disk access
- [x] Implement raw CD-ROM access
- [x] Implement FAT32 file system
- Commands:
    - [x] `clear`/`cls` command
    - [x] `echo` command
    - [x] `reboot`/`restart` command
    - [x] `poweroff`/`shutdown` command
    - [x] `exit`/`logout` command
    - [x] `ls` command
    - [x] `cat` command
    - [ ] `cd` command
- [ ] Implement ISO 9660 file system
