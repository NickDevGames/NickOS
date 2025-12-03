#!/bin/bash

./build.sh
qemu-system-i386 -cdrom build/NickOS.iso -boot d -debugcon file:/dev/stdout -m 256 -hda disk.img