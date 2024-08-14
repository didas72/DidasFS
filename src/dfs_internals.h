//dfs_internals.h - Declares functions for internal use to declutter didasFS.c

#ifndef DIDASFS_INTERNALS_H
#define DIDASFS_INTERNALS_H

#include <stdint.h>
#include <stddef.h>
#include <unistd.h>
#include <fcntl.h>

#include "dfs.h"
#include "dfs_structures.h"


#pragma region Block-Address abstraction
static size_t blk_idx_to_addr(const dfs_partition *partition, const blk_idx_t index);
static size_t blk_off_to_addr(const dfs_partition *partition, const blk_idx_t index, const size_t offset);
static size_t entry_loc_to_addr(const dfs_partition *partition, const entry_ptr_loc entry_loc);
static entry_ptr_loc get_root_loc();
#pragma endregion

#pragma region Device helpers
off_t device_seek(const int whence, const size_t offset, const dfs_partition *partition);
ssize_t device_write(void *buffer, const size_t len, const dfs_partition *partition);
ssize_t device_write_at(const size_t addr, void *buffer, const size_t len, const dfs_partition *partition);
ssize_t device_write_at_blk(const blk_idx_t index, void *buffer, const size_t len, const dfs_partition *partition);
ssize_t device_write_at_entry_loc(const entry_ptr_loc entry_loc, entry_pointer *buffer, const dfs_partition *partition);
ssize_t device_read(void *buffer, const size_t len, const dfs_partition *partition);
ssize_t device_read_at(const size_t addr, void *buffer, const size_t len, const dfs_partition *partition);
ssize_t device_read_at_blk(const blk_idx_t index, void *buffer, const size_t len, const dfs_partition *partition);
ssize_t device_read_at_entry_loc(const entry_ptr_loc entry_loc, void *buffer, const dfs_partition *partition);
static dfs_err force_allocate_space(const char *device, size_t size);
#pragma endregion

#pragma region Code naming
static size_t determine_first_blk_addr(uint32_t blk_count);
static size_t determine_size_from_blk_count(size_t blk_count);
static size_t determine_blk_count(size_t maxSize, size_t *partition_size);
static dfs_err init_empty_partition(const char *device, size_t blk_count);
static dfs_err validate_partition_header(const dfs_partition *pt);
static dfs_err set_stream_pos(dfs_partition *pt, const size_t position, dfs_file *file);
static dfs_err advance_stream(dfs_partition *pt, size_t offset, dfs_file *file);
static dfs_err rewind_stream(dfs_partition *pt, dfs_file *file);
#pragma endregion

#pragma region Block navigation
static dfs_err find_entry_ptr(const dfs_partition *pt, const char *path, entry_pointer *entry, entry_ptr_loc *entry_loc);
static dfs_err find_entry_ptr_recursion(const dfs_partition *pt, const blk_idx_t cur_blk, const char *path, entry_pointer *entry, entry_ptr_loc *entry_loc);
static dfs_err find_free_blk(const dfs_partition *pt, blk_idx_t *index);
#pragma endregion

#pragma region Block manipulation
static dfs_err append_blk_to_file(const dfs_partition *pt, const entry_ptr_loc entry_loc, blk_idx_t *new_blk_idx, dfs_file *handle);
static dfs_err append_entry_to_dir(const dfs_partition *pt, const entry_ptr_loc dir_entryLoc, entry_pointer new_entry);
#pragma endregion

#pragma region File manipulation
static dfs_err create_object(dfs_partition *pt, const char *path, const uint16_t flags);
static dfs_err determine_file_size(dfs_partition *pt, const entry_pointer entry, size_t *size);
static bool object_is_file(entry_pointer entry);
#pragma endregion

#pragma region File handles
static dfs_err handle_can_open(dfs_partition *pt, const char *path, const dfs_filem_flags flags, bool *can_open);
static dfs_err handle_get(dfs_partition *pt, const int descriptor, dfs_file **file);

static bool handle_open_flags_compatible(const dfs_filem_flags new, const dfs_filem_flags open);
static bool object_is_writable(entry_pointer entry);
#pragma endregion

#endif
