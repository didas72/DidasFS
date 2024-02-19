# TODO list

REMINDER: Cleanup TODO in code

TEST: File seeking

ADD FEATURE: Cleanup check (get number of open files, maybe also partitions)

FIX: Missing error on too long file/dir names
FIX: Object creation not accepting flags

ADD FEATURE:

* IO interface: ftruncate, fsync (flush)    [DONE: fopen, fread, fwrite, fclose]
* Management interface: rmdir, rmfile (rm)    [DONE: mkdir, create (touch)]

ADD FEATURE: Error checking partition header
ADD FEATURE: Partition layout allow bootloader

ADD FEATURE:

* Exists file/directory
* List files/directories
* Copy/move file
* Check total free space

PERFORMANCE: set_stream_pos should not seek from beggining
PERFORMANCE: Make blk_map searches in groups (byte sized for example)
PERFORMANCE: Make blk_map changes buffered
PERFORMANCE: Make reads/writes buffered
REMINDER: Ensure flushes when closing streams (both in FS and in system)

OPTIONAL FEATURES:

* Verify and cleanup FS
* Recover FS
