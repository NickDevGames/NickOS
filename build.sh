#!/bin/bash
set -xe
# This is a build script for the current setup.
# For larger projects, you may want to consider using GNU Make

# Create neccesary directories
mkdir -p build
mkdir -p iso/boot/grub

# Compile boot.asm with NASM
nasm -f elf32 src/boot.asm -o build/boot.o
nasm -f elf32 src/read_sector_stub.asm -o build/disk.o

# Compile the kernel with clang. (Getting a GCC compiler on Replit is difficult, but clang supports many binary formats out of the box.)
clang -c -target i686-none-elf -ffreestanding -mno-sse -Wall src/kernel.c -o build/kernel.o

# Link
ld -T linker.ld -o kernel.bin -static -nostdlib build/boot.o build/disk.o build/kernel.o -m elf_i386

# Clean
rm -rf build/*.o

# Make an ISO out of the kernel with grub-mkrescue
cp kernel.bin iso/boot/kernel.bin
cp grub.cfg iso/boot/grub/grub.cfg
grub-mkrescue -o build/NickOS.iso iso

rm -rf *.bin