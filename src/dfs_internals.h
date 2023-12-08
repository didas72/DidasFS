//didasFS_internals.h - Declares functions for internal use to declutter didasFS.c

#ifndef DIDASFS_INTERNALS_H
#define DIDASFS_INTERNALS_H

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>

#include "dfs.h"
#include "dfs_structures.h"


#pragma region Block-Address abstraction
size_t blk_idx_to_addr(const dfs_partition *partition, const blk_idx_t index);
size_t blk_off_to_addr(const dfs_partition *partition, const blk_idx_t index, const size_t offset);
size_t entry_loc_to_addr(const dfs_partition *partition, const entry_ptr_loc entryLoc);
entry_ptr_loc get_root_loc();
#pragma endregion

#pragma region Device helpers
off_t device_seek(const int whence, const size_t offset, const dfs_partition *partition);
ssize_t device_write(void *buffer, const size_t len, const dfs_partition *partition);
ssize_t device_write_at(const size_t addr, void *buffer, const size_t len, const dfs_partition *partition);
ssize_t device_write_at_blk(const blk_idx_t index, void *buffer, const size_t len, const dfs_partition *partition);
ssize_t device_write_at_entry_loc(const entry_ptr_loc entryLoc, entry_pointer *buffer, const dfs_partition *partition);
ssize_t device_read(void *buffer, const size_t len, const dfs_partition *partition);
ssize_t device_read_at(const size_t addr, void *buffer, const size_t len, const dfs_partition *partition);
ssize_t device_read_at_blk(const blk_idx_t index, void *buffer, const size_t len, const dfs_partition *partition);
ssize_t device_read_at_entry_loc(const entry_ptr_loc entryLoc, void *buffer, const dfs_partition *partition);
dfs_err force_allocate_space(const char *device, size_t size);
#pragma endregion

#pragma region Code naming
size_t determine_first_blk_addr(uint32_t blockMapSize);
size_t determine_size_from_blk_count(size_t blockCount);
size_t determine_blk_count(size_t maxSize, size_t *partitionSize);
dfs_err init_empty_partition(const char *device, size_t blockCount);
dfs_err validate_partition_header(const dfs_partition *pt);
dfs_err set_stream_pos(size_t position, DFileStream *fs);
#pragma endregion

#pragma region Block navigation
dfs_err find_entry_ptr(const dfs_partition *pt, const char *path, entry_pointer *entry, entry_ptr_loc *entryLoc);
dfs_err find_entry_ptr_recursion(const dfs_partition *pt, const blk_idx_t curBlock, const char *path, entry_pointer *entry, entry_ptr_loc *entryLoc);
dfs_err find_free_blk(const dfs_partition *pt, blk_idx_t *index);
#pragma endregion

#pragma region Block manipulation
dfs_err append_blk_to_file(const dfs_partition *pt, const entry_ptr_loc entryLoc, blk_idx_t *newBlkIdx);
dfs_err append_entry_to_dir(const dfs_partition *pt, const entry_ptr_loc dirEntryLoc, entry_pointer newEntry);
#pragma endregion

#pragma region File manipulation
dfs_err create_object(dfs_partition *pt, const char *path, const uint16_t flags);
dfs_err determine_file_size(dfs_partition *pt, const entry_pointer entry, size_t *size);
#pragma endregion

#pragma region File handles
dfs_err handle_can_open(dfs_partition *pt, const char *path, const dfs_filem_flags flags);
dfs_err handle_open(dfs_partition *pt, const char *path, const dfs_filem_flags flags, int *handle);
dfs_err handle_close(dfs_partition *pt, const int handle);
dfs_err handle_get(dfs_partition *pt, const int handle, dfs_file *fh);

int file_descriptor_hasher(void *descriptor);
void dfilestream_deallocator(void *stream);
#pragma endregion

#endif
