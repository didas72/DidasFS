# TODO list

REMINDER: Cleanup TODO in code

FIX: File creation not accepting flags
FIX: Writing to a file must update all related handles

ADD FEATURE: Implement target interfaces:

* IO interface: fopen, fread, fwrite, fclose, ftruncate, fsync (flush)
* Management interface: mkdir, rmdir, create (touch), rmfile (rm)

TEST: File functions
TEST: Directory functions

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
