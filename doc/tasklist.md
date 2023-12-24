# TODO list

REMINDER: Cleanup TODO in code

TEST: File functions
TEST: File seeking

FIX: Error on too long file/dir names
FIX: Object creation not accepting flags

ADD FEATURE: Implement target interfaces:

* IO interface: fopen, fread, fwrite, fclose, ftruncate, fsync (flush)
* Management interface: mkdir, rmdir, create (touch), rmfile (rm)

ADD FEATURE: Error checking partition header
ADD FEATURE: Partition layout allow bootloader

ADD FEATURE: Implement higher level:

* List files/directories
* Exists file/directory
* Copy file
* Move dile/dir
* Check total free space
* Verify and cleanup FS
* Recover FS

PERFORMANCE: Make blk_map changes buffered
PERFORMANCE: Make reads/writes buffered
PERFORMANCE: Remove system buffering
REMINDER: Ensure flushes when closing streams (both in FS and in system)

FIX: ?Needed? Replace all fread to work with 512 multiples for hardware devices
