# NickOS

If you run the `dbgdisk` command, the system will hang. This is because there is no disk attached. First, you need to create a disk image using e.g. [dd](https://www.gnu.org/software/coreutils/manual/html_node/dd-invocation.html) and attach it to QEMU using the `-hda` argument.
