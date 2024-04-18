//dfs.c - Implements dfs.h

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <stddef.h>

#include "dfs.h"
#include "dfs_structures.h"
#include "dfs_internals.h"
#include "paths.h"
#include "err_utils.h"



static int log_level = DFS_LOG_ERROR;



//============================
//= Function implementations =
//============================
#pragma region Function implementations
void dfs_set_log_level(int level)
{
	log_level = level;
}

dfs_err dfs_pcreate(const char *device, size_t total_size)
{
	//FIXME: Allows creation of >max size (cannot allow block_count to be 0xFFFFFFFF)
	ERR_NULL(device, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(device));
	ERR_IF(total_size == 0, DFS_NVAL_ARGS, "Argument 'total_size' cannot be 0.\n");

	dfs_err err;
	size_t size;
	size_t blk_count = determine_blk_count(total_size, &size);
	ERR_IF(size == 0, DFS_NVAL_ARGS, "Invalid data size %ld.\n", total_size);

	ERR_NZERO((err = force_allocate_space(device, size)), err, "Failed to allocate space.\n");
	ERR_NZERO((err = init_empty_partition(device, blk_count)), err, "Failed to init empty partition.\n");

	return DFS_SUCCESS;
}

dfs_err dfs_popen(const char *device, dfs_partition **pt)
{
	ERR_NULL(device, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(device));
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));

	*pt = NULL;
	dfs_err err;

	dfs_partition* ptr = malloc(sizeof(dfs_partition));
	ERR_NULL(ptr, DFS_FAILED_ALLOC, ERR_MSG_ALLOC_FAIL);

	ptr->device = open(device, O_RDWR | O_SYNC);
	ERR_IF_FREE1(ptr->device == -1, DFS_FAILED_DEVICE_OPEN, ptr, "Failed to open device %s.\n", device);

	ERR_NZERO_CLEANUP_FREE1((err = validate_partition_header(ptr)), err,
		close(ptr->device), ptr, "Partition header validation failed.\n");

	//Determine address of root block for fast access
	uint32_t usage_map_size;
	lseek(ptr->device, 4, SEEK_SET);
	ssize_t readc = read(ptr->device, &usage_map_size, sizeof(uint32_t));
	ERR_IF_CLEANUP_FREE1(readc != sizeof(uint32_t), DFS_FAILED_DEVICE_READ,
		close(ptr->device), ptr, ERR_MSG_DEVICE_READ_FAIL);

	ptr->root_blk_addr = determine_first_blk_addr(usage_map_size);
	ptr->blk_count = usage_map_size << 3;

	ERR_NZERO_CLEANUP_FREE1((err = load_blk_map(ptr)), err, close(ptr->device), ptr, "Failed to load block map.\n");

	*pt = ptr;
	return DFS_SUCCESS;
}

dfs_err dfs_pclose(dfs_partition *pt)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));

	dfs_err err;

	ERR_NZERO((err = flush_full_blk_map(pt)), err, "Failed to flush block map.\n");
	ERR_NZERO((err = destroy_blk_map(pt)), err, "Failed to destroy block map.\n");
	close(pt->device);
	free(pt);

	return DFS_SUCCESS;
}

dfs_err dfs_dcreate(dfs_partition *pt, const char *path)
{
	return create_object(pt, path, ENTRY_FLAG_DIR | ENTRY_FLAG_READWRITE);
}

dfs_err dfs_fcreate(dfs_partition *pt, const char *path)
{
	return create_object(pt, path, ENTRY_FLAG_FILE | ENTRY_FLAG_READWRITE);
}

dfs_err dfs_fopen(dfs_partition *pt, const char *path, const dfs_filem_flags flags, int *descriptor)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(path, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(path));
	ERR_NULL(descriptor, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(descriptor));

	ERR_IF(dfs_path_is_empty(path), DFS_NVAL_PATH, ERR_MSG_EMPTY_PATH("open"));
	ERR_IF(flags == 0, DFS_NVAL_ARGS, ERR_MSG_NVAL_FLAGS("file opening"));

	*descriptor = -1;

	int new_descriptor = get_lowest_unused_descriptor(*pt);
	ERR_IF(new_descriptor == -1, DFS_MAX_HANDLES_REACHED, "Reached maximum number of open handles.\n");

	dfs_err err;
	bool can_open = false;
	ERR_IF((err = handle_can_open(pt, path, flags, &can_open)), err, "Failed to test if file '%s' can be opened.\n", path);
	ERR_IF(!can_open, DFS_UNAUTHORIZED_ACCESS, ERR_MSG_UNAUTHORIZED_ACCESS("open", path));

	entry_pointer entry;
	entry_ptr_loc entry_loc;
	ERR_NZERO((err = find_entry_ptr(pt, path, &entry, &entry_loc)), err, "Could not find entry for file '%s'.\n", path);
	ERR_IF(!object_is_file(entry) || !object_is_writable(entry), DFS_UNAUTHORIZED_ACCESS, ERR_MSG_UNAUTHORIZED_ACCESS("open", path));

	dfs_file handle = {
		.present = true,
		.flags = flags,
		.head = 0,
		.cur_blk_idx = entry.first_blk,
		.first_blk_idx = entry.first_blk,
		.entry_loc = entry_loc
	};

	strcpy(handle.path, path);

	pt->open_handles[new_descriptor] = handle;
	*descriptor = new_descriptor;
	return DFS_SUCCESS;
}

dfs_err dfs_fclose(dfs_partition *pt, const int descriptor)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));

	dfs_err err;
	dfs_file *file;
	handle_get(pt, descriptor, &file);
	ERR_IF((err = handle_get(pt, descriptor, &file)), err, ERR_MSG_HANDLE_FETCH_FAIL(descriptor));

	file->present = false;

	return DFS_SUCCESS;
}

dfs_err dfs_fwrite(dfs_partition *pt, const int descriptor, const void *buffer, const size_t len, size_t *written)
{ //TODO: Maybe break down into smaller functions
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(buffer, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(buffer));
	ERR_IF(len == 0, DFS_NVAL_ARGS, "Argument 'len' must not be 0.\n");

	size_t buffer_head = 0;
	ssize_t readc;
	block_header cur_blk;
	dfs_err err;
	dfs_file *file;
	handle_get(pt, descriptor, &file);
	ERR_IF((err = handle_get(pt, descriptor, &file)), err, ERR_MSG_HANDLE_FETCH_FAIL(descriptor));
	
	//Should work for both append and overwrite
	while (buffer_head < len)
	{
		//Read cur block
		readc = device_read_at_blk(file->cur_blk_idx, &cur_blk, sizeof(block_header), pt);
		ERR_IF(readc != sizeof(block_header), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

		//Calculate metrics
		size_t block_offset = file->head % BLOCK_DATA_SIZE;
		size_t data_addr = blk_off_to_addr(pt, file->cur_blk_idx, block_offset);
		size_t max_write = BLOCK_DATA_SIZE - block_offset;
		size_t pending_write = len - buffer_head;
		size_t cur_blk_write = (max_write < pending_write) ? max_write : pending_write;
		size_t final_new_data_in_blk = block_offset + cur_blk_write;
		size_t final_data_in_blk = (cur_blk.used_space > final_new_data_in_blk) ? cur_blk.used_space : final_new_data_in_blk;

		if (final_new_data_in_blk > cur_blk.used_space) //If grown
		{
			//Update cur block
			cur_blk.used_space = final_data_in_blk;

			//Flush block changes
			readc = device_write_at_blk(file->cur_blk_idx, &cur_blk, sizeof(block_header), pt);
			ERR_IF(readc != sizeof(block_header), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);
		}

		//Flush data
		readc = device_write_at(data_addr, &((char*)buffer)[buffer_head], cur_blk_write, pt);
		ERR_NZERO(readc != (ssize_t)cur_blk_write, DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);

		//Update head
		buffer_head += cur_blk_write;
		file->head += cur_blk_write;

		//Advance to next block if needed
		if (buffer_head < len)
		{
			file->cur_blk_idx = cur_blk.next_blk;

			//Grow file if needed
			if (!file->cur_blk_idx)
			{
				uint32_t new_index;
				err = append_blk_to_file(pt, file->entry_loc, &new_index);
				ERR_NZERO(err, err, "Failed to grow file.\n");
				file->cur_blk_idx = new_index;
			}
		}
	}

	if (written)
		*written = buffer_head;

	return DFS_SUCCESS;
}

dfs_err dfs_fread(dfs_partition *pt, const int descriptor, const void *buffer, const size_t len, size_t *read)
{ //TODO: Maybe break down into smaller functions
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(buffer, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(buffer));
	ERR_IF(len == 0, DFS_NVAL_ARGS, "Argument 'len' must not be 0.\n");

	size_t buffer_head = 0, readB;
	block_header cur_blk;
	dfs_err err;
	dfs_file *file;
	ERR_IF((err = handle_get(pt, descriptor, &file)), err, ERR_MSG_HANDLE_FETCH_FAIL(descriptor));

	while (buffer_head < len)
	{
		//Read cur block
		readB = device_read_at_blk(file->cur_blk_idx, &cur_blk, sizeof(block_header), pt);
		ERR_IF(readB != sizeof(block_header), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

		//Calculate metrics
		size_t blockOffset = file->head % BLOCK_DATA_SIZE;
		size_t dataAddr = blk_off_to_addr(pt, file->cur_blk_idx, blockOffset);
		size_t maxRead = BLOCK_DATA_SIZE - blockOffset;
		size_t pendingRead = len - buffer_head;
		size_t cur_blkRead = (maxRead < pendingRead) ? maxRead : pendingRead;
		
		//Read data
		readB = device_read_at(dataAddr, &((char*)buffer)[buffer_head], cur_blkRead, pt);
		ERR_NZERO(readB != cur_blkRead, DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);
	
		//Update head
		buffer_head += cur_blkRead;
		file->head += cur_blkRead;

		//Advance to next block if needed
		if (buffer_head < len)
		{
			file->cur_blk_idx = cur_blk.next_blk;

			//Return early if file ended
			if (!file->cur_blk_idx)
			{
				if (read)
					*read = buffer_head;
				return DFS_SUCCESS;
			}
		}
	}

	if (read)
		*read = buffer_head;

	return DFS_SUCCESS;
}

dfs_err dfs_fset_pos(dfs_partition *pt, const int descriptor, const size_t pos)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));

	dfs_err err;
	dfs_file *file;
	ERR_IF((err = handle_get(pt, descriptor, &file)), err, ERR_MSG_HANDLE_FETCH_FAIL(descriptor));

	return set_stream_pos(pt, pos, file);
}

dfs_err dfs_fget_pos(dfs_partition *pt, const int descriptor, size_t *pos)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(pos, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pos));

	dfs_err err;
	dfs_file *file;
	ERR_IF((err = handle_get(pt, descriptor, &file)), err, ERR_MSG_HANDLE_FETCH_FAIL(descriptor));

	*pos = file->head;

	return DFS_SUCCESS;
}

dfs_err dfs_dlist_entries(dfs_partition *pt, const char *path, size_t capacity, dfs_entry *entries, size_t *count)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(path, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(path));
	ERR_IF(!entries && capacity, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(entries));

	ssize_t readc;
	entry_pointer ptr;
	dfs_err err;
	ERR_NZERO((err = find_entry_ptr(pt, path, &ptr, NULL)), err, "Could not find entry for directory '%s'.\n", path);
	ERR_IF(!(ptr.flags & ENTRY_FLAG_DIR), DFS_NVAL_PATH, "Can only list entries of a directory (a file was provided).\n");

	size_t entries_found = 0, head = 0;
	block_header cur_blk;
	blk_idx_t blk_idx = ptr.first_blk;
	entry_pointer cur_entry;
	do //If first is 0 then root block was used. All entries are given a non-zero blk_idx at creation time
	{
		readc = device_read_at_blk(blk_idx, &cur_blk, sizeof(block_header), pt); //TODO: Error checking
		ERR_IF(readc != sizeof(block_header), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

		size_t entries_in_blk = cur_blk.used_space / sizeof(entry_pointer);
		
		for (size_t i = 0; i < entries_in_blk && head < capacity; i++, head++)
		{
			device_read_at(
				blk_off_to_addr(pt, blk_idx, i * sizeof(entry_pointer)),
				&cur_entry, sizeof(entry_pointer), pt);

			entries[head].dir = cur_entry.flags & ENTRY_FLAG_DIR;
			err = determine_file_size(pt, cur_entry, &entries[head].length);
			memcpy(entries[head].name, &cur_entry.name, MAX_PATH_NAME);
		}

		entries_found += entries_in_blk;
		blk_idx = cur_blk.next_blk;
	} while (blk_idx);

	if (count)
		*count = entries_found;

	return DFS_SUCCESS;
}
#pragma endregion


//========================================
//= _structures function implementations =
//========================================
#pragma region _structures function implementations
int get_lowest_unused_descriptor(const dfs_partition pt)
{
	for (int i = 0; i < DFS_MAX_HANDLES; i++)
	{
		if (!pt.open_handles[i].present)
			return i;
	}

	return -1;
}

dfs_err load_blk_map(dfs_partition* host)
{
	ERR_NULL(host, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(host));

	blk_map *map = malloc(sizeof(blk_map));

	ERR_NULL(map, DFS_FAILED_ALLOC, ERR_MSG_ALLOC_FAIL);

	map->length = host->blk_count >> 3;
	map->map = malloc(map->length);

	ERR_NULL_FREE1(map->map, DFS_FAILED_ALLOC, map, ERR_MSG_ALLOC_FAIL);

	ssize_t readc = device_read_at(sizeof(partition_header), map->map, map->length, host);

	ERR_IF_FREE2(readc != (ssize_t)map->length, DFS_FAILED_DEVICE_READ, map->map, map, ERR_MSG_DEVICE_READ_FAIL);

	host->usage_map = map;

	return DFS_SUCCESS;
}

dfs_err get_blk_used(const dfs_partition *pt, blk_idx_t blk_idx, bool *used)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(used, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(used));
	ERR_IF(blk_idx >= pt->blk_count, DFS_NVAL_ARGS, "Argument 'blk_idx' must be smaller than total block count.\n");

	blk_idx_t offset = blk_idx & 0x7; 
	blk_idx_t index = blk_idx & ~0x7;

	*used = pt->usage_map->map[index] & (1 << offset);

	return DFS_SUCCESS;
}

dfs_err set_blk_used(const dfs_partition *pt, blk_idx_t blk_idx, bool used)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_IF(blk_idx >= pt->blk_count, DFS_NVAL_ARGS, "Argument 'blk_idx' must be smaller than total block count.\n");

	blk_idx_t offset = blk_idx & 0x7; 
	blk_idx_t index = blk_idx & ~0x7;

	if (used)
		pt->usage_map->map[index] |= (1 << offset);
	else
		pt->usage_map->map[index] &= ~(1 << offset);

	return DFS_SUCCESS;
}

dfs_err flush_full_blk_map(const dfs_partition *pt)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));

	device_seek(SEEK_SET, sizeof(partition_header), pt);
	size_t written = device_write(pt->usage_map->map, pt->usage_map->length, pt);

	ERR_IF(written != pt->usage_map->length, DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);

	return DFS_SUCCESS;
}

dfs_err destroy_blk_map(dfs_partition *pt)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));

	free(pt->usage_map->map);
	free(pt->usage_map);

	return DFS_SUCCESS;
}

int file_descriptor_hasher(const void *descriptor)
{
	return *((int*)descriptor);
}

void dfs_file_deallocator(void *file)
{
	dfs_file *fs = (dfs_file*)file;

	free(fs);
}
#pragma endregion


//=====================================
//= Internal function implementations =
//=====================================
#pragma region Device helpers
inline off_t device_seek(const int whence, const size_t offset, const dfs_partition *partition)
{
	return lseek(partition->device, offset, whence);
}

inline ssize_t device_write(void *buffer, const size_t len, const dfs_partition *partition)
{
	return write(partition->device, buffer, len);
}
inline ssize_t device_write_at(const size_t addr, void *buffer, const size_t len, const dfs_partition *partition)
{
	device_seek(SEEK_SET, addr, partition);
	return device_write(buffer, len, partition);
}
inline ssize_t device_write_at_blk(const blk_idx_t index, void *buffer, const size_t len, const dfs_partition *partition)
{
	size_t addr = blk_idx_to_addr(partition, index);
	device_seek(SEEK_SET, addr, partition);
	return device_write(buffer, len, partition);
}
inline ssize_t device_write_at_entry_loc(const entry_ptr_loc entry_loc, entry_pointer *buffer, const dfs_partition *partition)
{
	size_t addr = entry_loc_to_addr(partition, entry_loc);
	device_seek(SEEK_SET, addr, partition);
	return device_write(buffer, sizeof(entry_pointer), partition);
}

inline ssize_t device_read(void *buffer, const size_t len, const dfs_partition *partition)
{
	return read(partition->device, buffer, len);
}
inline ssize_t device_read_at(const size_t addr, void *buffer, const size_t len, const dfs_partition *partition)
{
	device_seek(SEEK_SET, addr, partition);
	return device_read(buffer, len, partition);
}
inline ssize_t device_read_at_blk(const blk_idx_t index, void *buffer, const size_t len, const dfs_partition *partition)
{
	size_t addr = blk_idx_to_addr(partition, index);
	device_seek(SEEK_SET, addr, partition);
	return device_read(buffer, len, partition);
}
inline ssize_t device_read_at_entry_loc(const entry_ptr_loc entry_loc, void *buffer, const dfs_partition *partition)
{
	size_t addr = entry_loc_to_addr(partition, entry_loc);
	device_seek(SEEK_SET, addr, partition);
	return device_read(buffer, sizeof(entry_pointer), partition);
}

dfs_err force_allocate_space(const char *device, size_t size)
{
	ERR_NULL(device, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(device));
	ERR_IF(size == 0, DFS_NVAL_ARGS, "Argument 'size' must not be 0.\n");

	int file = open(device, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	char zero = 0;

	ERR_IF(file == -1, DFS_FAILED_DEVICE_OPEN, ERR_MSG_DEVICE_OPEN_FAIL);

	lseek(file, size - 1, SEEK_SET);
	ssize_t writtec = write(file, &zero, 1);
	ERR_IF(writtec == -1, DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);
	close(file);

	return DFS_SUCCESS;
}
#pragma endregion
#pragma region Block-Address abstraction
size_t blk_idx_to_addr(const dfs_partition *partition, const blk_idx_t index)
{
	return partition->root_blk_addr + (size_t)index * BLOCK_SIZE;
}

size_t blk_off_to_addr(const dfs_partition *partition, const blk_idx_t index, const size_t offset)
{
	return blk_idx_to_addr(partition, index) + sizeof(block_header) + offset;
}

size_t entry_loc_to_addr(const dfs_partition *partition, const entry_ptr_loc entry_loc)
{
	if (entry_loc.blk_idx == ~0u && entry_loc.entry_idx == ~0u)
		return partition->root_blk_addr - sizeof(entry_pointer);

	size_t base_address = blk_idx_to_addr(partition, entry_loc.blk_idx);
	size_t offset = sizeof(block_header) + sizeof(entry_pointer) * entry_loc.entry_idx;

	return base_address + offset;
}

entry_ptr_loc get_root_loc()
{
	entry_ptr_loc loc = { .blk_idx = ~0u, .entry_idx = ~0u };
	return loc;
}
#pragma endregion
#pragma region Code naming
size_t determine_first_blk_addr(uint32_t blk_count)
{
	size_t without_pad = (size_t)(blk_count >> 3) + sizeof(partition_header) + sizeof(entry_pointer);
	size_t rem = without_pad & (SECTOR_SIZE - 1);
	return without_pad + (rem ? SECTOR_SIZE - rem : 0);
}

size_t determine_size_from_blk_count(size_t blk_count)
{
	size_t usage_map_size = blk_count >> 3;
	size_t dts = determine_first_blk_addr(usage_map_size);
	return dts + blk_count * BLOCK_SIZE;
}

size_t determine_blk_count(size_t maxSize, size_t *partition_size)
{
	size_t max = MAX_BLKS, min = 0;
	size_t current = 0, total_size = 0;
	int iter_count = 0; //Iteration limiter, should not be needed
	
	while (iter_count < 64)
	{
		iter_count++;

		current = (min + max) >> 1;
		total_size = determine_size_from_blk_count(current);

		if (total_size < maxSize)
			min = current;
		else if (total_size > maxSize)
			max = current;
		else
			goto return_value;

		if (max - min <= 1)
		{
			current = min;
			goto return_value;
		}
	}

	return ~0;

return_value:
	if (partition_size)
		*partition_size = determine_size_from_blk_count(current);

	//FIXME: Returned value is not multiple of 8, breaks partition header

	return current;
}

dfs_err init_empty_partition(const char *device, size_t blk_count)
{
	ERR_NULL(device, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(device));
	ERR_IF(blk_count == 0, DFS_NVAL_ARGS, "Argument 'blk_count' must not be 0.\n");

	int file = open(device, O_RDWR | O_SYNC);

	ERR_IF(file == -1, DFS_NVAL_ARGS, ERR_MSG_DEVICE_OPEN_FAIL);

	//Write header
	partition_header header = { .magic_number = MAGIC_NUMBER,
		.block_count = blk_count };
	size_t written = write(file, &header, sizeof(partition_header));
	ERR_IF_CLEANUP(written != sizeof(partition_header),
		DFS_FAILED_DEVICE_WRITE, close(file), ERR_MSG_DEVICE_WRITE_FAIL);

	//Clear block map
	char zeros[512] = { 0 };
	written = write(file, zeros, 512 - sizeof(partition_header));
	ERR_IF_CLEANUP(written != 512 - sizeof(partition_header),
		DFS_FAILED_DEVICE_WRITE, close(file), ERR_MSG_DEVICE_WRITE_FAIL);

	for (size_t i = 1; ((512 - sizeof(partition_header)) << 3) + (i << 12) < blk_count; i++)
	{
		written = write(file, zeros, 512);
		ERR_IF_CLEANUP(written != 512,
			DFS_FAILED_DEVICE_WRITE, close(file), ERR_MSG_DEVICE_WRITE_FAIL);
	}

	//Set root to used
	char root_used = 1;
	lseek(file, sizeof(partition_header), SEEK_SET);
	written = write(file, &root_used, 1);
	ERR_IF_CLEANUP(written != 1,
		DFS_FAILED_DEVICE_WRITE, close(file), ERR_MSG_DEVICE_WRITE_FAIL);

	//Set root entry_pointer
	entry_pointer root_pointer = { .first_blk = 0, .last_blk = 0, .flags = ENTRY_FLAG_DIR, .resvd = 0, .name = {0} };
	memcpy(root_pointer.name, "FSRoot!PlsNoTouchy:)", MAX_PATH_NAME);
	lseek(file, determine_first_blk_addr(header.block_count) - sizeof(entry_pointer), SEEK_SET);
	written = write(file, &root_pointer, sizeof(root_pointer));
	ERR_IF_CLEANUP(written != sizeof(entry_pointer),
		DFS_FAILED_DEVICE_WRITE, close(file), ERR_MSG_DEVICE_WRITE_FAIL);

	//Init root block header
	block_header root_header = { 0 };
	//Seeker should be at root block already
	written = write(file, &root_header, sizeof(block_header));
	ERR_IF_CLEANUP(written != sizeof(block_header),
		DFS_FAILED_DEVICE_WRITE, close(file), ERR_MSG_DEVICE_WRITE_FAIL);

	close(file);

	return DFS_SUCCESS;
}

dfs_err validate_partition_header(const dfs_partition* pt)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));

	partition_header buff;
	ssize_t readc;
	
	readc = read(pt->device, &buff, sizeof(partition_header));
	ERR_IF(readc != sizeof(partition_header), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

	//Check magic number
	ERR_IF(buff.magic_number != MAGIC_NUMBER, DFS_CORRUPTED_PARTITION, "Partition has incorrect magic number.\n");

	//Check reserved
	ERR_IF(buff.resvd != 0, DFS_CORRUPTED_PARTITION, "Partition has reserved flags set.\n");

	return DFS_SUCCESS;
}

dfs_err set_stream_pos(dfs_partition *pt, const size_t position, dfs_file *file)
{ //TODO: Maybe break down into smaller functions
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(file, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(file));

	size_t cur_pos = 0, left;
	uint32_t cur_blk = file->first_blk_idx;
	block_header cur_header;
	ssize_t readc;
	dfs_err err;

	while (cur_pos != position)
	{
		left = position - cur_pos;
		readc = device_read_at_blk(cur_blk, &cur_header, sizeof(block_header), pt);
		ERR_IF(readc != sizeof(block_header), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

		if (left >= BLOCK_DATA_SIZE) //Full block skip
		{
			if (cur_header.next_blk)
			{
				//REVIEW: Might not be needed, if next_blk is set then all space should be in use
				//Update used space if neeed
				if (cur_header.used_space < BLOCK_DATA_SIZE)
				{
					cur_header.used_space = BLOCK_DATA_SIZE;
					readc = device_write_at_blk(cur_blk, &cur_header, sizeof(block_header), pt);
					ERR_IF(readc != sizeof(block_header), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);
				}

				//Advance block
				cur_blk = cur_header.next_blk;
			}
			else
			{
				//FIXME: Might need to increase used space

				//Grow file
				uint32_t new_index;
				err = append_blk_to_file(pt, file->entry_loc, &new_index);
				ERR_NZERO(err, err, "Failed to grow file.\n");
				cur_blk = new_index;
			}
			cur_pos += BLOCK_DATA_SIZE;
		}
		else //Partial block advance
		{
			if (left > cur_header.used_space) //Grow used space
			{
				cur_header.used_space = left;
				readc = device_write_at_blk(cur_blk, &cur_header, sizeof(block_header), pt);
				ERR_IF(readc != sizeof(block_header), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);
			}
			cur_pos += left;
		}
	}

	file->cur_blk_idx = cur_blk;
	file->head = cur_pos;

	return DFS_SUCCESS;
}
#pragma endregion
#pragma region Block navigation
dfs_err find_entry_ptr(const dfs_partition *pt, const char *path, entry_pointer *entry, entry_ptr_loc *entry_loc)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(path, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(path));

	if (dfs_path_is_empty(path))
	{
		entry_ptr_loc loc = get_root_loc();

		if (entry)
		{
			entry_pointer e;
			ssize_t readc = device_read_at_entry_loc(loc, &e, pt);
			ERR_IF(readc != sizeof(entry_pointer), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);
			*entry = e;
		}

		if (entry_loc)
			*entry_loc = loc;

		return DFS_SUCCESS;
	}

	return find_entry_ptr_recursion(pt, 0, path, entry, entry_loc);
}

dfs_err find_entry_ptr_recursion(const dfs_partition* pt, const blk_idx_t cur_blk, const char *path, entry_pointer *entry, entry_ptr_loc *entry_loc)
{ //TODO: Maybe break down into smaller functions
	char root[MAX_PATH_NAME + 1];
	char tail[MAX_PATH + 1];
	char *search_name;
	entry_pointer *entries = NULL, found_entry = { 0 };
	entry_ptr_loc location = { 0 };
	block_header cur_header = { 0 };
	uint32_t next_idx = 0;
	ssize_t readc;

	memset(root, 0, MAX_PATH_NAME + 1);
	memset(tail, 0, MAX_PATH + 1);

	dfs_path_get_root(root, path);
	dfs_path_get_tail(tail, path);

	search_name = dfs_path_is_empty(root) ? tail : root;

	//Read current block entries
	device_seek(SEEK_SET, blk_idx_to_addr(pt, cur_blk), pt); //TODO: Handle errors
	readc = device_read(&cur_header, sizeof(block_header), pt);
	ERR_IF(readc != sizeof(block_header), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

	entries = malloc(ENTRIES_PER_BLK * sizeof(entry_pointer));
	ERR_NULL(entries, DFS_FAILED_ALLOC, ERR_MSG_ALLOC_FAIL);

	readc = device_read(entries, ENTRIES_PER_BLK * sizeof(entry_pointer), pt);
	ERR_IF(readc != ENTRIES_PER_BLK * sizeof(entry_pointer), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

	//TODO: Possibly validate header used_space is multiple of sizeof(entry_pointer)
	int valid_entry_count = cur_header.used_space / sizeof(entry_pointer);

	//Search entries for dir/file
	for (int i = 0; i < valid_entry_count; i++)
	{
		//Filter out not matching names (at most one should match)
		if (strncmp(search_name, entries[i].name, MAX_PATH_NAME))
			continue;

		next_idx = entries[i].first_blk;
		found_entry = entries[i];
		location.blk_idx = cur_blk;
		location.entry_idx = i;
		break;
	}

	free(entries);

	if (!next_idx) //Couldn't find dir/file in current block
	{
		if (!cur_header.next_blk) return DFS_PATH_NOT_FOUND;

		return find_entry_ptr_recursion(pt, cur_header.next_blk, path, entry, entry_loc);
	}
	else //Found dir/file
	{
		if (next_idx && strlen(root)) //It's not final, continue search on next block
			return find_entry_ptr_recursion(pt, next_idx, tail, entry, entry_loc);
		else //It's the requested dir/file, return index
		{
			if (entry)
				*entry = found_entry;
			if (entry_loc)
				*entry_loc = location;
			return DFS_SUCCESS;
		}
	}
}

dfs_err find_free_blk(const dfs_partition *pt, blk_idx_t *index)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(index, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(index));

	dfs_err err;
	uint32_t i;

	//TODO: Optimize to use usage_map byte searches (8 bits at a time)
	for (i = 0; i < pt->blk_count; i++)
	{
		bool used;
		ERR_NZERO((err = get_blk_used(pt, i, &used)), err, "Failed to retrieve block usage state.\n");

		if (!used)
		{
			*index = i;
			return DFS_SUCCESS;
		}
	}

	ERR(DFS_NO_SPACE, "Failed to allocate space for new block.\n");
}
#pragma endregion
#pragma region Block manipulation
dfs_err append_blk_to_file(const dfs_partition *pt, const entry_ptr_loc entry_loc, blk_idx_t *new_idx)
{ //TODO: Maybe break down into smaller functions
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));

	dfs_err err;
	ssize_t readc;
	uint32_t new_blk_idx, old_block_idx;
	entry_pointer entry;
	block_header new_blk, old_block;

	//Find block and reserve
	ERR_NZERO((err = find_free_blk(pt, &new_blk_idx)), err, "Could not find a free block.\n");
	ERR_NZERO((err = set_blk_used(pt, new_blk_idx, true)), err, "Coult not flag block as used.\n");
	ERR_NZERO((err = flush_full_blk_map(pt)), err, "Failed to flush block map.\n");

	//Read entry pointer
	readc = device_read_at_entry_loc(entry_loc, &entry, pt);
	ERR_IF(readc != sizeof(entry_pointer), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

	//Update last block index in entry
	old_block_idx = entry.last_blk;
	entry.last_blk = new_blk_idx;

	//Flush changes
	readc = device_write_at_entry_loc(entry_loc, &entry, pt);
	ERR_IF(readc != sizeof(entry_pointer), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);

	//Create new block header
	new_blk.next_blk = 0;
	new_blk.prev_blk = old_block_idx;
	new_blk.used_space = 0;
	new_blk.resvd = 0;

	//Store new block
	readc = device_write_at_blk(new_blk_idx, &new_blk, sizeof(block_header), pt);
	ERR_IF(readc != sizeof(block_header), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);

	//Read old block
	readc = device_read_at_blk(old_block_idx, &old_block, sizeof(block_header), pt);
	ERR_IF(readc != sizeof(block_header), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

	//Update index
	old_block.next_blk = new_blk_idx;

	//Flush changes
	readc = device_write_at_blk(old_block_idx, &old_block, sizeof(block_header), pt);
	ERR_IF(readc != sizeof(block_header), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);

	if (new_idx)
		*new_idx = new_blk_idx;

	return DFS_SUCCESS;
}

dfs_err append_entry_to_dir(const dfs_partition *pt, const entry_ptr_loc dir_entryLoc, entry_pointer new_entry)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));

	entry_pointer dir_entry;
	block_header dir_blk;
	uint32_t blk_idx;
	dfs_err err;
	ssize_t readc;

	//Read dir entry
	readc = device_read_at_entry_loc(dir_entryLoc, &dir_entry, pt);
	ERR_IF(readc != sizeof(entry_pointer), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

	//Load last block
	blk_idx = dir_entry.last_blk;
	readc = device_read_at_blk(blk_idx, &dir_blk, sizeof(block_header), pt);
	ERR_IF(readc != sizeof(block_header), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

	if (dir_blk.used_space + sizeof(entry_pointer) >= BLOCK_DATA_SIZE) //No free space
	{
		//Append block and update block index
		ERR_NZERO((err = append_blk_to_file(pt, dir_entryLoc, &blk_idx)), err, "Failed to append block to file.\n");

		//Load new block
		readc = device_read_at_blk(blk_idx, &dir_blk, sizeof(block_header), pt);
		ERR_IF(readc != sizeof(block_header), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);
	}

	//Write new entry pointer
	size_t addr = blk_off_to_addr(pt, blk_idx, dir_blk.used_space);
	readc = device_write_at(addr, &new_entry, sizeof(entry_pointer), pt);
	ERR_IF(readc != sizeof(entry_pointer), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);

	//Update block header
	dir_blk.used_space += (uint32_t)sizeof(entry_pointer);

	//Flush header changes
	readc = device_write_at_blk(blk_idx, &dir_blk, sizeof(block_header), pt);
	ERR_IF(readc != sizeof(block_header), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);

	return DFS_SUCCESS;
}
#pragma endregion
#pragma region File manipulation
dfs_err create_object(dfs_partition *pt, const char *path, const uint16_t flags)
{ //TODO: Maybe break down into smaller functions
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(path, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(path));
	
	char parent_dir[MAX_PATH + 1];
	char name[MAX_PATH_NAME + 1];

	dfs_err err;
	entry_pointer new_entry = { 0 };
	entry_ptr_loc parent_loc;
	block_header new_blk;
	uint32_t new_blk_idx;
	ssize_t readc;

	ERR_IF(dfs_path_is_empty(path), DFS_NVAL_PATH, "Cannot create root object. (The provided path was empty)\n");

	err = find_entry_ptr(pt, path, NULL, NULL);
	ERR_IF(err == DFS_SUCCESS, DFS_ALREADY_EXISTS, ERR_MSG_ALREADY_EXISTS(path));

	dfs_path_get_parent(parent_dir, path);
	dfs_path_get_name(name, path);

	//Find parent dir
	ERR_NZERO((err = find_entry_ptr(pt, parent_dir, NULL, &parent_loc)), err, "Could not find parent directory.\n");

	//Check if parent is a directory
	entry_pointer parent;
	readc = device_read_at_entry_loc(parent_loc, &parent, pt);
	ERR_IF(readc != sizeof(entry_pointer), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);
	ERR_IF(!(parent.flags & ENTRY_FLAG_DIR), DFS_NVAL_PATH, "Cannot create object inside a file.\n");

	//Find and reserve free block
	ERR_NZERO((err = find_free_blk(pt, &new_blk_idx)), err, "Could not find free block.\n");
	ERR_NZERO((err = set_blk_used(pt, new_blk_idx, true)), err, "Could not flag block as used.\n");
	ERR_NZERO((err = flush_full_blk_map(pt)), err, "Could not flush block map.\n");

	//Create new entry
	memset(new_entry.name, 0, MAX_PATH_NAME);
	strncpy(new_entry.name, name, MAX_PATH_NAME);
	new_entry.first_blk = new_blk_idx;
	new_entry.last_blk = new_blk_idx;
	new_entry.resvd = 0;
	new_entry.flags = flags;

	//Append entry to parent
	ERR_NZERO((err = append_entry_to_dir(pt, parent_loc, new_entry)), err, "Could not append entry to directory.\n");

	//Set new block header
	new_blk.next_blk = 0;
	new_blk.prev_blk = 0;
	new_blk.used_space = 0;
	new_blk.resvd = 0;

	//Flush changes
	readc = device_write_at_blk(new_blk_idx, &new_blk, sizeof(block_header), pt);
	ERR_IF(readc != sizeof(block_header), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);

	return DFS_SUCCESS;
}

dfs_err determine_file_size(dfs_partition *pt, const entry_pointer entry, size_t *size)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(size, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(size));

	size_t counter = 0;
	ssize_t readc;
	block_header cur_blk;
	uint32_t blk_idx = entry.first_blk;

	while (blk_idx) //First should always go through since there should be no entries with null/root block
	{
		readc = device_read_at_blk(blk_idx, &cur_blk, sizeof(block_header), pt);
		ERR_IF(readc != sizeof(block_header), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

		counter += cur_blk.used_space; //In theory all but last should be full
		blk_idx = cur_blk.next_blk;
	}
	
	*size = counter;

	return DFS_SUCCESS;
}

bool object_is_file(entry_pointer entry)
{
	return !(entry.flags & ENTRY_FLAG_DIR);
}

bool object_is_writable(entry_pointer entry)
{
	return !(entry.flags & ENTRY_FLAG_READONLY);
}
#pragma endregion
#pragma region File handles
dfs_err handle_can_open(dfs_partition *pt, const char *path, const dfs_filem_flags flags, bool *can_open)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(path, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(path));
	ERR_NULL(can_open, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(can_open));

	bool compatible = true;
	for (size_t i = 0; i < DFS_MAX_HANDLES; i++)
	{
		dfs_file cur = pt->open_handles[i];

		if (!cur.present)
			continue;

		if (strcmp(cur.path, path))
			continue;

		if (!handle_open_flags_compatible(flags, cur.flags))
		{
			compatible = false;
			break;
		}
	}

	*can_open = compatible;
	return DFS_SUCCESS;
}

dfs_err handle_get(dfs_partition *pt, const int descriptor, dfs_file **file)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(file, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(file));
	ERR_IF(descriptor < 0, DFS_NVAL_DESCRIPTOR, ERR_MSG_NVAL_DESCRIPTOR("access", descriptor));
	ERR_IF(descriptor > DFS_MAX_HANDLES, DFS_NVAL_DESCRIPTOR, ERR_MSG_NVAL_DESCRIPTOR("access", descriptor));
	ERR_IF(!pt->open_handles[descriptor].present, DFS_NVAL_DESCRIPTOR, ERR_MSG_NVAL_DESCRIPTOR("access", descriptor));

	*file = &pt->open_handles[descriptor];
	return DFS_SUCCESS;
}

bool handle_open_flags_compatible(const dfs_filem_flags new, const dfs_filem_flags open)
{
	if ((new & DFS_FILEM_READ) && !(open & DFS_FILEM_SHARE_READ))
		return false;
	if ((new & DFS_FILEM_WRITE) && !(open & DFS_FILEM_SHARE_WRITE))
		return false;

	return true;
}
#pragma endregion
