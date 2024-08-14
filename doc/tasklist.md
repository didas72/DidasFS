# TODO list

REMINDER: Cleanup TODO in code

FIX:

* File mode used on opening but ignored on read/write
* Missing error on too long file/dir names
* Object creation not accepting flags

ADD FEATURE:

* ftruncate
* rmdir/rm
* **Ensure to check dir block used_size when removing objects, might break dlist_entries and searching**
* cp/mv
* Check total free space (statvfs?)
* fsync (flush) \[once buffering is implemented\]

ADD FEATURE:

* Cleanup check (get number of open files, maybe also partitions)
* Error checking partition header

PERFORMANCE:

* set_stream_pos should not seek from beggining
* Make blk_map searches in groups (byte sized for example)
* Make blk_map changes buffered
* Make reads/writes buffered
* **Ensure flushes when closing streams (both in FS and in system)**

OPTIONAL FEATURES:

* Partition layout allow bootloader
* Verify and cleanup FS
* Recover FS
