#!/bin/bash

./build.sh
qemu-system-i386 -cdrom build/NickOS.iso -boot d -debugcon file:/dev/stdout -hda disk.img