//didasFS_internals.h - Declares functions for internal use to declutter didasFS.c

#ifndef DIDASFS_INTERNALS_H
#define DIDASFS_INTERNALS_H

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

#include "didasFS.h"
#include "didasFS_structures.h"


#pragma region Block-Address abstraction
size_t blk_idx_to_addr(const DPartition *partition, const uint32_t index);
size_t blk_off_to_addr(const DPartition *partition, const uint32_t index, const size_t offset);
size_t entry_loc_to_addr(const DPartition *partition, const EntryPointerLoc entryLoc);
EntryPointerLoc get_root_loc();
#pragma endregion

#pragma region Device helpers
int device_seek(const int whence, const size_t offset, const DPartition *partition);
ssize_t device_write(void *buffer, const size_t len, const DPartition *partition);
ssize_t device_write_at(const size_t addr, void *buffer, const size_t len, const DPartition *partition);
ssize_t device_write_at_blk(const uint32_t index, void *buffer, const size_t len, const DPartition *partition);
ssize_t device_write_at_entry_loc(const EntryPointerLoc entryLoc, EntryPointer *buffer, const DPartition *partition);
ssize_t device_read(void *buffer, const size_t len, const DPartition *partition);
ssize_t device_read_at(const size_t addr, void *buffer, const size_t len, const DPartition *partition);
ssize_t device_read_at_blk(const uint32_t index, void *buffer, const size_t len, const DPartition *partition);
ssize_t device_read_at_entry_loc(const EntryPointerLoc entryLoc, void *buffer, const DPartition *partition);
int force_allocate_space(const char *device, size_t size);
#pragma endregion

#pragma region Code naming
size_t determine_first_blk_addr(uint32_t blockMapSize);
size_t determine_size_from_blk_count(size_t blockCount);
size_t determine_blk_count(size_t maxSize, size_t *partitionSize);
int init_empty_partition(const char *device, size_t blockCount);
int validate_partition_header(const DPartition *pt);
int set_stream_pos(size_t position, DFileStream *fs);
#pragma endregion

#pragma region Block navigation
int find_entry_ptr(const DPartition *pt, const char *path, EntryPointer *entry, EntryPointerLoc *entryLoc);
int find_entry_ptr_recursion(const DPartition *pt, const uint32_t curBlock, const char *path, EntryPointer *entry, EntryPointerLoc *entryLoc);
int find_free_blk(const DPartition *pt, uint32_t *index);
#pragma endregion

#pragma region Block manipulation
int append_blk_to_file(const DPartition *pt, const EntryPointerLoc entryLoc, uint32_t *newBlkIdx);
int append_entry_to_dir(const DPartition *pt, const EntryPointerLoc dirEntryLoc, EntryPointer newEntry);
#pragma endregion

#pragma region File manipulation
int create_object(DPartition *pt, const char *path, const uint16_t flags);
int determine_file_size(DPartition *pt, const EntryPointer entry, size_t *size);
#pragma endregion

#endif
