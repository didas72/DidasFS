//didasFS.c - Implements DidasFS.h

#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#include "didasFS.h"
#include "didasFS_structures.h"
#include "didasFS_internals.h"
#include "dPaths.h"
#include "errUtils.h"



#define BASE_FILEHANDLE_CAPACITY 64



//============================
//= Function implementations =
//============================
#pragma region Function implementations
DFS_err dpcreate(const char *device, size_t totalSize)
{
	ERR_NULL(device, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(device));
	ERR_IF(totalSize == 0, DFS_NVAL_ARGS, "Argument 'totalSize' cannot be 0.\n");

	int err;
	size_t size;
	size_t blockCount = determine_blk_count(totalSize, &size);

	ERR_IF(size == 0, DFS_NVAL_ARGS, "Invalid data size %ld.\n", dataSize);
	ERR_NZERO((err = force_allocate_space(device, size)), err, "Failed to allocate space.\n");

	ERR_NZERO((err = init_empty_partition(device, blockCount)), err, "Failed to init empty partition.\n");

	return DFS_SUCCESS;
}

DFS_err dpopen(const char *device, DPartition **ptHandle)
{
	ERR_NULL(ptHandle, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(ptHandle));

	*ptHandle = NULL;
	int err;

	DPartition* pt = malloc(sizeof(DPartition));
	ERR_NULL(pt, DFS_FAILED_ALLOC, ERR_MSG_ALLOC_FAIL);

	pt->device = open(device, O_RDWR | O_SYNC);
	ERR_IF_FREE1(pt->device == -1, DFS_FAILED_DEVICE_OPEN, pt, "Failed to open device %s.\n", device);

	ERR_NZERO_CLEANUP_FREE1((err = validate_partition_header(pt)), err,
		close(pt->device), pt, "Partition header validation failed.\n");

	//Determine address of root block for fast access
	uint32_t blockMapSize;
	lseek(pt->device, 4, SEEK_SET);
	ssize_t readc = read(pt->device, &blockMapSize, sizeof(uint32_t));
	ERR_IF_CLEANUP_FREE1(readc != sizeof(uint32_t), DFS_FAILED_DEVICE_READ,
		close(pt->device), pt, ERR_MSG_DEVICE_READ_FAIL);

	pt->rootBlockAddr = determine_first_blk_addr(blockMapSize);
	pt->blockCount = blockMapSize << 3;

	ERR_NZERO_CLEANUP_FREE1((err = load_blk_map(pt)), err, close(pt->device), pt, "Failed to load block map.\n");

	pt->fileHandles = hashmap_create(file_descriptor_hasher, dfilestream_deallocator, BASE_FILEHANDLE_CAPACITY);
	ERR_NULL_CLEANUP_FREE1(pt->fileHandles, DFS_FAILED_ALLOC, close(pt->device); destroy_blk_map(pt), pt, ERR_MSG_ALLOC_FAIL);

	*ptHandle = pt;
	return DFS_SUCCESS;
}

DFS_err dpclose(DPartition *ptHandle)
{
	ERR_NULL(ptHandle, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(ptHandle));

	int err;

	ERR_NZERO((err = flush_full_blk_map(ptHandle)), err, "Failed to flush block map.\n");
	ERR_NZERO((err = destroy_blk_map(ptHandle)), err, "Failed to destroy block map.\n");
	close(ptHandle->device);
	free(ptHandle);

	return DFS_SUCCESS;
}

DFS_err ddcreate(DPartition *pt, const char *path)
{
	return create_object(pt, path, ENTRY_FLAG_DIR | ENTRY_FLAG_READWRITE);
}

DFS_err dfcreate(DPartition *pt, const char *path)
{
	return create_object(pt, path, ENTRY_FLAG_FILE | ENTRY_FLAG_READWRITE);
}

DFS_err dfopen(DPartition *pt, const char *path, DFileStream **fsHandle)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(fsHandle, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(fsHandle));

	*fsHandle = NULL;
	int err;
	EntryPointer entry;
	EntryPointerLoc entryLoc;

	DFileStream* fs = malloc(sizeof(DFileStream));
	ERR_NULL(fs, DFS_FAILED_ALLOC, ERR_MSG_ALLOC_FAIL);

	ERR_NZERO_FREE1((err = find_entry_ptr(pt, path, &entry, &entryLoc)), err, fs, "Could not find entry for the requested file.\n");
	
	ERR_NZERO_FREE1((err = determine_file_size(pt, entry, &fs->fileSize)), err, fs, "Failed to determine file size.\n");
	fs->filePos = 0;
	fs->curBlockIdx = entry.firstBlock;
	fs->firstBlockIdx = entry.firstBlock;
	fs->lastBlockIdx = entry.lastBlock;
	fs->entryLoc = entryLoc;
	fs->pt = pt;

	*fsHandle = fs;
	return DFS_SUCCESS;
}

DFS_err dfclose(DFileStream *fs)
{
	ERR_NULL(fs, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(fs));

	//TODO: Handle pt stuff with fs->pt

	//Will not be needed, deallocated by hashmap
	free(fs);
	
	return DFS_SUCCESS;
}

DFS_err dfwrite(void *buffer, size_t len, DFileStream *fs, size_t *written)
{
	ERR_NULL(buffer, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(buffer));
	ERR_NULL(fs, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(fs));
	ERR_IF(len == 0, DFS_NVAL_ARGS, "Argument 'len' must not be 0.\n");

	size_t bufferHead = 0;
	ssize_t readc;
	BlockHeader curBlock;
	int err;
	
	//Should work for both append and overwrite
	while (bufferHead < len)
	{
		//Read cur block
		readc = device_read_at_blk(fs->curBlockIdx, &curBlock, sizeof(BlockHeader), fs->pt);
		ERR_IF(readc != sizeof(BlockHeader), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL); //TODO: Revert changes?

		//Calculate metrics
		size_t blockOffset = fs->filePos % BLOCK_DATA_SIZE;
		size_t dataAddr = blk_off_to_addr(fs->pt, fs->curBlockIdx, blockOffset);
		size_t maxWrite = BLOCK_DATA_SIZE - blockOffset;
		size_t pendingWrite = len - bufferHead;
		size_t curBlockWrite = (maxWrite < pendingWrite) ? maxWrite : pendingWrite;
		size_t finalNewDataInBlock = blockOffset + curBlockWrite;
		size_t finalDataInBlock = (curBlock.usedSpace > finalNewDataInBlock) ? curBlock.usedSpace : finalNewDataInBlock;

		if (finalNewDataInBlock > curBlock.usedSpace) //If grown
		{
			fs->fileSize += finalNewDataInBlock - curBlock.usedSpace;

			//Update cur block
			curBlock.usedSpace = finalDataInBlock;

			//Flush block changes
			readc = device_write_at_blk(fs->curBlockIdx, &curBlock, sizeof(BlockHeader), fs->pt);
			ERR_IF(readc != sizeof(BlockHeader), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL); //TODO: Revert changes?
		}

		//Flush data
		readc = device_write_at(dataAddr, &((char*)buffer)[bufferHead], curBlockWrite, fs->pt);
		ERR_NZERO(readc != (ssize_t)curBlockWrite, DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL); //TODO: Revert changes?

		//Update head
		bufferHead += curBlockWrite;
		fs->filePos += curBlockWrite;

		//Advance to next block if needed
		if (bufferHead < len)
		{
			fs->curBlockIdx = curBlock.nextBlock;

			//Grow file if needed
			if (!fs->curBlockIdx)
			{
				uint32_t newIndex;
				err = append_blk_to_file(fs->pt, fs->entryLoc, &newIndex);
				ERR_NZERO(err, err, "Failed to grow file.\n");
				fs->curBlockIdx = fs->lastBlockIdx = newIndex;
			}
		}
	}

	if (written)
		*written = bufferHead;

	return DFS_SUCCESS;
}

DFS_err dfread(void *buffer, size_t len, DFileStream *fs, size_t *readc)
{
	ERR_NULL(buffer, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(buffer));
	ERR_NULL(fs, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(fs));
	ERR_IF(len == 0, DFS_NVAL_ARGS, "Argument 'len' must not be 0.\n");

	size_t bufferHead = 0, readB;
	BlockHeader curBlock;

	while (bufferHead < len)
	{
		//Read cur block
		readB = device_read_at_blk(fs->curBlockIdx, &curBlock, sizeof(BlockHeader), fs->pt);
		ERR_IF(readB != sizeof(BlockHeader), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL); //TODO: Revert changes?

		//Calculate metrics
		size_t blockOffset = fs->filePos % BLOCK_DATA_SIZE;
		size_t dataAddr = blk_off_to_addr(fs->pt, fs->curBlockIdx, blockOffset);
		size_t maxRead = BLOCK_DATA_SIZE - blockOffset;
		size_t pendingRead = len - bufferHead;
		size_t curBlockRead = (maxRead < pendingRead) ? maxRead : pendingRead;
		
		//Read data
		readB = device_read_at(dataAddr, &((char*)buffer)[bufferHead], curBlockRead, fs->pt);
		ERR_NZERO(readB != curBlockRead, DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL); //TODO: Revert changes?
	
		//Update head
		bufferHead += curBlockRead;
		fs->filePos += curBlockRead;

		//Advance to next block if needed
		if (bufferHead < len)
		{
			fs->curBlockIdx = curBlock.nextBlock;

			//Return early if file ended
			if (!fs->curBlockIdx)
			{
				if (readc)
					*readc = bufferHead;
				return DFS_SUCCESS;
			}
		}
	}

	if (readc)
		*readc = bufferHead;

	return DFS_SUCCESS;
}

DFS_err dfseek(size_t pos, DFileStream *fs)
{
	ERR_NULL(fs, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(fs));

	return set_stream_pos(pos, fs);
}
#pragma endregion


//========================================
//= _structures function implementations =
//========================================
#pragma region _structures function implementations
DFS_err load_blk_map(DPartition* host)
{
	ERR_NULL(host, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(host));

	BlockMap *map = malloc(sizeof(BlockMap));

	ERR_NULL(map, DFS_FAILED_ALLOC, ERR_MSG_ALLOC_FAIL);

	map->host = host;
	map->length = host->blockCount >> 3;
	map->map = malloc(map->length);

	ERR_NULL_FREE1(map->map, DFS_FAILED_ALLOC, map, ERR_MSG_ALLOC_FAIL);

	ssize_t readc = device_read_at(sizeof(PartitionHeader), map->map, map->length, host);

	ERR_IF_FREE2(readc != (ssize_t)map->length, DFS_FAILED_DEVICE_READ, map->map, map, ERR_MSG_DEVICE_READ_FAIL);

	host->blockMap = map;

	return DFS_SUCCESS;
}

DFS_err get_blk_used(const DPartition *pt, uint32_t blockIndex, bool *used)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(used, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(used));
	ERR_IF(blockIndex >= pt->blockCount, DFS_NVAL_ARGS, "Argument 'blockIndex' must be smaller than total block count.\n");

	uint32_t offset = blockIndex & 0x7; 
	uint32_t index = blockIndex & ~0x7;

	*used = pt->blockMap->map[index] & (1 << offset);

	return DFS_SUCCESS;
}

DFS_err set_blk_used(const DPartition *pt, uint32_t blockIndex, bool used)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_IF(blockIndex >= pt->blockCount, DFS_NVAL_ARGS, "Argument 'blockIndex' must be smaller than total block count.\n");

	uint32_t offset = blockIndex & 0x7; 
	uint32_t index = blockIndex & ~0x7;

	if (used)
		pt->blockMap->map[index] |= (1 << offset);
	else
		pt->blockMap->map[index] &= ~(1 << offset);

	return DFS_SUCCESS;
}

DFS_err flush_full_blk_map(const DPartition *pt)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));

	device_seek(SEEK_SET, sizeof(PartitionHeader), pt);
	size_t written = device_write(pt->blockMap->map, pt->blockMap->length, pt);

	ERR_IF(written != pt->blockMap->length, DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);

	return DFS_SUCCESS;
}

DFS_err destroy_blk_map(DPartition *pt)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));

	free(pt->blockMap->map);
	free(pt->blockMap);

	return DFS_SUCCESS;
}

int file_descriptor_hasher(void *descriptor)
{
	return *((int*)descriptor);
}

void dfilestream_deallocator(void *stream)
{
	DFileStream *fs = (DFileStream*)stream;

	free(fs);
}
#pragma endregion


//=====================================
//= Internal function implementations =
//=====================================
#pragma region Device helpers
inline off_t device_seek(const int whence, const size_t offset, const DPartition *partition)
{
	return lseek(partition->device, offset, whence);
}

inline ssize_t device_write(void *buffer, const size_t len, const DPartition *partition)
{
	return write(partition->device, buffer, len);
}
inline ssize_t device_write_at(const size_t addr, void *buffer, const size_t len, const DPartition *partition)
{
	device_seek(SEEK_SET, addr, partition);
	return device_write(buffer, len, partition);
}
inline ssize_t device_write_at_blk(const blk_idx_t index, void *buffer, const size_t len, const DPartition *partition)
{
	size_t addr = blk_idx_to_addr(partition, index);
	device_seek(SEEK_SET, addr, partition);
	return device_write(buffer, len, partition);
}
inline ssize_t device_write_at_entry_loc(const EntryPointerLoc entryLoc, EntryPointer *buffer, const DPartition *partition)
{
	size_t addr = entry_loc_to_addr(partition, entryLoc);
	device_seek(SEEK_SET, addr, partition);
	return device_write(buffer, sizeof(EntryPointer), partition);
}

inline ssize_t device_read(void *buffer, const size_t len, const DPartition *partition)
{
	return read(partition->device, buffer, len);
}
inline ssize_t device_read_at(const size_t addr, void *buffer, const size_t len, const DPartition *partition)
{
	device_seek(SEEK_SET, addr, partition);
	return device_read(buffer, len, partition);
}
inline ssize_t device_read_at_blk(const blk_idx_t index, void *buffer, const size_t len, const DPartition *partition)
{
	size_t addr = blk_idx_to_addr(partition, index);
	device_seek(SEEK_SET, addr, partition);
	return device_read(buffer, len, partition);
}
inline ssize_t device_read_at_entry_loc(const EntryPointerLoc entryLoc, void *buffer, const DPartition *partition)
{
	size_t addr = entry_loc_to_addr(partition, entryLoc);
	device_seek(SEEK_SET, addr, partition);
	return device_read(buffer, sizeof(EntryPointer), partition);
}

DFS_err force_allocate_space(const char *device, size_t size)
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
size_t blk_idx_to_addr(const DPartition *partition, const blk_idx_t index)
{
	return partition->rootBlockAddr + (size_t)index * BLOCK_SIZE;
}

size_t blk_off_to_addr(const DPartition *partition, const blk_idx_t index, const size_t offset)
{
	return blk_idx_to_addr(partition, index) + sizeof(BlockHeader) + offset;
}

size_t entry_loc_to_addr(const DPartition *partition, const EntryPointerLoc entryLoc)
{
	if (entryLoc.blockIndex == ~0u && entryLoc.entryIndex == ~0u)
		return partition->rootBlockAddr - sizeof(EntryPointer);

	size_t baseAddress = blk_idx_to_addr(partition, entryLoc.blockIndex);
	size_t offset = sizeof(BlockHeader) + sizeof(EntryPointer) * entryLoc.entryIndex;

	return baseAddress + offset;
}

EntryPointerLoc get_root_loc()
{
	EntryPointerLoc loc = { .blockIndex = ~0u, .entryIndex = ~0u };
	return loc;
}
#pragma endregion
#pragma region Code naming
size_t determine_first_blk_addr(uint32_t blockMapSize)
{
	size_t withoutPad = (size_t)blockMapSize + sizeof(PartitionHeader) + sizeof(EntryPointer);
	size_t rem = withoutPad & (SECTOR_SIZE - 1);

	return withoutPad + (rem ? SECTOR_SIZE - rem : 0);
}

size_t determine_size_from_blk_count(size_t blockCount)
{
	size_t blockMapSize = blockCount >> 3;

	//DTS = DaTa Start
	size_t DTS = determine_first_blk_addr(blockMapSize);

	return DTS + blockCount * BLOCK_SIZE;
}

size_t determine_blk_count(size_t maxSize, size_t *partitionSize)
{
	size_t max = MAX_BLKS, min = 0;
	size_t current = 0, totalSize = 0;
	int iterCount = 0; //Iteration limiter, should not be needed
	
	while (iterCount < 64)
	{
		iterCount++;

		current = (min + max) >> 1;
		totalSize = determine_size_from_blk_count(current);

		if (totalSize < maxSize)
			min = current;
		else if (totalSize > maxSize)
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
	if (partitionSize)
		*partitionSize = determine_size_from_blk_count(current);

	return current;
}

DFS_err init_empty_partition(const char *device, size_t blockCount)
{
	ERR_NULL(device, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(device));
	ERR_IF(blockCount == 0, DFS_NVAL_ARGS, "Argument 'blockCount' must not be 0.\n");

	int file = open(device, O_RDWR | O_SYNC);

	ERR_IF(file == -1, DFS_NVAL_ARGS, ERR_MSG_DEVICE_OPEN_FAIL);

	//Write header
	PartitionHeader header = { .magicNumber = MAGIC_NUMBER,
		.blockMapSize = blockCount >> 3 };

	size_t written = write(file, &header, sizeof(PartitionHeader));

	ERR_IF_CLEANUP(written != sizeof(PartitionHeader),
		DFS_FAILED_DEVICE_WRITE, close(file), ERR_MSG_DEVICE_WRITE_FAIL);

	char rootUsed = 1;
	written = write(file, &rootUsed, 1);
	ERR_IF_CLEANUP(written != 1,
		DFS_FAILED_DEVICE_WRITE, close(file), ERR_MSG_DEVICE_WRITE_FAIL);

	close(file);

	return DFS_SUCCESS;
}

DFS_err validate_partition_header(const DPartition* pt)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));

	PartitionHeader buff;
	ssize_t readc;
	
	readc = read(pt->device, &buff, sizeof(PartitionHeader));
	ERR_IF(readc != sizeof(PartitionHeader), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

	//Check magic number
	ERR_IF(buff.magicNumber != MAGIC_NUMBER, DFS_CORRUPTED_FS, "Partition has incorrect magic number.\n");

	//Check reserved
	ERR_IF(buff.resvd != 0, DFS_CORRUPTED_FS, "Partition has reserved flags set.\n");

	//Check blockCount will not overflow
	ERR_IF(buff.blockMapSize > (~0u >> 3), DFS_CORRUPTED_FS, "Partition blockMapSize is too big.\n");

	return DFS_SUCCESS;
}

DFS_err set_stream_pos(size_t position, DFileStream *fs)
{
	ERR_IF(position > fs->fileSize, DFS_NVAL_SEEK, "Attempted to seek beyond file boundaries.\n");

	size_t curPos = 0, left;
	uint32_t curBlock = fs->firstBlockIdx;
	BlockHeader curHeader;
	ssize_t readc;

	while (curPos != position)
	{
		left = curPos - position;
		readc = device_read_at_blk(curBlock, &curHeader, sizeof(BlockHeader), fs->pt);
		ERR_IF(readc != sizeof(BlockHeader), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

		if (left >= BLOCK_DATA_SIZE) //Full block skip
		{
			//No need to check for next block, checked by first ERR_IF

			position += BLOCK_DATA_SIZE;
			curBlock = curHeader.nextBlock;
		}
		else //Partial block advance
		{
			//No need to check within block usedSize, checked by first ERR_IF
			position += left;
		}
	}

	fs->curBlockIdx = curBlock;
	fs->filePos = curPos;

	return DFS_SUCCESS;
}
#pragma endregion
#pragma region Block navigation
DFS_err find_entry_ptr(const DPartition* pt, const char *path, EntryPointer *entry, EntryPointerLoc *entryLoc)
{
	//This is the 'intermediate' method
	//Validates arguments and initiates recursion

	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(path, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(path));

	if (DPathIsEmpty(path))
	{
		EntryPointerLoc loc = get_root_loc();

		if (entry)
		{
			EntryPointer e;
			ssize_t readc = device_read_at_entry_loc(loc, &e, pt);
			ERR_IF(readc != sizeof(EntryPointer), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);
			*entry = e;
		}

		if (entryLoc)
			*entryLoc = loc;

		return DFS_SUCCESS;
	}

	return find_entry_ptr_recursion(pt, 0, path, entry, entryLoc);
}

DFS_err find_entry_ptr_recursion(const DPartition* pt, const blk_idx_t curBlock, const char *path, EntryPointer *entry, EntryPointerLoc *entryLoc)
{
	char root[MAX_PATH_NAME + 1];
	char tail[MAX_PATH + 1];
	char *searchName;
	char searchForDir;
	EntryPointer *entries = NULL, foundEntry = { 0 };
	EntryPointerLoc location = { 0 };
	BlockHeader curHeader = { 0 };
	uint32_t nextIdx = 0;
	ssize_t readc;

	memset(root, 0, MAX_PATH_NAME + 1);
	memset(tail, 0, MAX_PATH + 1);

	DPathGetRoot(root, path);
	DPathGetTail(tail, path);

	if (!DPathIsEmpty(root)) //Looking for dir
	{
		searchForDir = ENTRY_FLAG_DIR;
		searchName = root;
	}
	else //Looking for file
	{
		searchForDir = 0;
		searchName = tail;
	}

	//Read current block entries
	device_seek(SEEK_SET, blk_idx_to_addr(pt, curBlock), pt); //TODO: Handle errors
	readc = device_read(&curHeader, sizeof(BlockHeader), pt);
	ERR_IF(readc != sizeof(BlockHeader), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL)

	entries = malloc(ENTRIES_PER_BLK * sizeof(EntryPointer));
	ERR_NULL(entries, DFS_FAILED_ALLOC, ERR_MSG_ALLOC_FAIL);

	readc = device_read(entries, ENTRIES_PER_BLK * sizeof(EntryPointer), pt); //TODO: Handle errors
	ERR_IF(readc != ENTRIES_PER_BLK * sizeof(EntryPointer), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL)

	//TODO: Possibly validate header usedSpace is multiple of sizeof(EntryPointer)
	int validEntryCount = curHeader.usedSpace / sizeof(EntryPointer);

	//Search entries for dir/file
	for (int i = 0; i < validEntryCount; i++)
	{
		//Filter out dir/files as needed
		if ((entries[i].flags & ENTRY_FLAG_DIR) != searchForDir)
			continue;

		//Filter out not matching names (at most one should match)
		if (strncmp(searchName, entries[i].name, MAX_PATH_NAME))
			continue;

		nextIdx = entries[i].firstBlock;
		foundEntry = entries[i];
		location.blockIndex = curBlock;
		location.entryIndex = i;
		break;
	}

	free(entries);

	if (!nextIdx) //Couldn't find dir/file in current block
	{
		ERR_IF(!curHeader.nextBlock, DFS_PATH_NOT_FOUND, "Could not find '%s' in '%s'.\n", searchName, path);

		return find_entry_ptr_recursion(pt, curHeader.nextBlock, path, entry, entryLoc);
	}
	else //Found dir/file
	{
		if (nextIdx && strlen(root)) //It's not final, continue search on next block
			return find_entry_ptr_recursion(pt, nextIdx, tail, entry, entryLoc);
		else //It's the requested dir/file, return index
		{
			if (entry)
				*entry = foundEntry;
			if (entryLoc)
				*entryLoc = location;
			return DFS_SUCCESS;
		}
	}
}

DFS_err find_free_blk(const DPartition *pt, blk_idx_t *index)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(index, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(index));

	int err;
	uint32_t i;

	//TODO: Optimize to use blockMap byte searches (8 bits at a time)
	for (i = 0; i < pt->blockCount; i++)
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
DFS_err append_blk_to_file(const DPartition *pt, const EntryPointerLoc entryLoc, blk_idx_t *newBlkIdx)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));

	int err; ssize_t readc;
	uint32_t newBlockIndex, oldBlockIndex;
	EntryPointer entry;
	BlockHeader newBlock, oldBlock;

	//Find block and reserve
	ERR_NZERO((err = find_free_blk(pt, &newBlockIndex)), err, "Could not find a free block.\n");
	ERR_NZERO((err = set_blk_used(pt, newBlockIndex, true)), err, "Coult not flag block as used.\n");
	ERR_NZERO((err = flush_full_blk_map(pt)), err, "Failed to flush block map.\n");

	//Read entry pointer
	readc = device_read_at_entry_loc(entryLoc, &entry, pt);
	ERR_IF(readc != sizeof(EntryPointer), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL); //TODO: Revert block set to used

	//Update last block index in entry
	oldBlockIndex = entry.lastBlock;
	entry.lastBlock = newBlockIndex;

	//Flush changes
	readc = device_write_at_entry_loc(entryLoc, &entry, pt);
	ERR_IF(readc != sizeof(EntryPointer), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL); //TODO: Revert block set to used

	//Create new block header
	newBlock.nextBlock = 0;
	newBlock.prevBlock = oldBlockIndex;
	newBlock.usedSpace = 0;
	newBlock.resvd = 0;

	//Store new block
	readc = device_write_at_blk(newBlockIndex, &newBlock, sizeof(BlockHeader), pt);
	ERR_IF(readc != sizeof(BlockHeader), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);

	//Read old block
	readc = device_read_at_blk(oldBlockIndex, &oldBlock, sizeof(BlockHeader), pt);
	ERR_IF(readc != sizeof(BlockHeader), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

	//Update index
	oldBlock.nextBlock = newBlockIndex;

	//Flush changes
	readc = device_write_at_blk(oldBlockIndex, &oldBlock, sizeof(BlockHeader), pt);
	ERR_IF(readc != sizeof(BlockHeader), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);

	if (newBlkIdx)
		*newBlkIdx = newBlockIndex;

	return DFS_SUCCESS;
}

DFS_err append_entry_to_dir(const DPartition *pt, const EntryPointerLoc dirEntryLoc, EntryPointer newEntry)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));

	EntryPointer dirEntry;
	BlockHeader dirBlock;
	uint32_t blockIndex;
	int err; ssize_t readc;

	//Read dir entry
	readc = device_read_at_entry_loc(dirEntryLoc, &dirEntry, pt);
	ERR_IF(readc != sizeof(EntryPointer), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

	//Load last block
	blockIndex = dirEntry.lastBlock;
	readc = device_read_at_blk(blockIndex, &dirBlock, sizeof(BlockHeader), pt);
	ERR_IF(readc != sizeof(BlockHeader), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

	if (dirBlock.usedSpace + sizeof(EntryPointer) >= BLOCK_DATA_SIZE) //No free space
	{
		//Append block and update block index
		ERR_NZERO((err = append_blk_to_file(pt, dirEntryLoc, &blockIndex)), err, "Failed to append block to file.\n");

		//Load new block
		readc = device_read_at_blk(blockIndex, &dirBlock, sizeof(BlockHeader), pt);
		ERR_IF(readc != sizeof(BlockHeader), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL); //TODO: Revert changes
	}

	//Write new entry pointer
	size_t addr = blk_off_to_addr(pt, blockIndex, dirBlock.usedSpace);
	readc = device_write_at(addr, &newEntry, sizeof(EntryPointer), pt);
	ERR_IF(readc != sizeof(EntryPointer), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL); //TODO: Revert changes

	//Update block header
	dirBlock.usedSpace += (uint32_t)sizeof(EntryPointer);

	//Flush header changes
	readc = device_write_at_blk(blockIndex, &dirBlock, sizeof(BlockHeader), pt);
	ERR_IF(readc != sizeof(BlockHeader), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL); //TODO: Revert changes

	return DFS_SUCCESS;
}
#pragma endregion
#pragma region File manipulation
DFS_err create_object(DPartition *pt, const char *path, const uint16_t flags)
{
	char parentDir[MAX_PATH + 1];
	char name[MAX_PATH_NAME + 1];

	int err;
	EntryPointer newEntry = { 0 };
	EntryPointerLoc parentLoc;
	BlockHeader newBlock;
	uint32_t newBlockIndex;
	ssize_t readc;

	DPathGetParent(parentDir, path);
	DPathGetName(name, path);

	//Find parent dir
	ERR_NZERO((err = find_entry_ptr(pt, parentDir, NULL, &parentLoc)), err, "Could not find entry for the requested file.\n");

	//Find and reserve free block
	ERR_NZERO((err = find_free_blk(pt, &newBlockIndex)), err, "Could not find free block.\n");
	ERR_NZERO((err = set_blk_used(pt, newBlockIndex, true)), err, "Could not flag block as used.\n");
	ERR_NZERO((err = flush_full_blk_map(pt)), err, "Could not flush block map.\n");

	//Create new entry
	memset(newEntry.name, 0, MAX_PATH_NAME);
	strncpy(newEntry.name, name, MAX_PATH_NAME);
	newEntry.firstBlock = newBlockIndex;
	newEntry.lastBlock = newBlockIndex;
	newEntry.resvd = 0;
	newEntry.flags = flags;

	//Append entry to parent
	ERR_NZERO((err = append_entry_to_dir(pt, parentLoc, newEntry)), err, "Could not append entry to directory.\n");

	//Set new block header
	newBlock.nextBlock = 0;
	newBlock.prevBlock = 0;
	newBlock.usedSpace = 0;
	newBlock.resvd = 0;

	//Flush changes
	readc = device_write_at_blk(newBlockIndex, &newBlock, sizeof(BlockHeader), pt);
	ERR_IF(readc != sizeof(BlockHeader), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);

	return DFS_SUCCESS;
}

DFS_err determine_file_size(DPartition *pt, const EntryPointer entry, size_t *size)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(size, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(size));

	size_t counter = 0;
	ssize_t readc;
	BlockHeader curBlock;
	uint32_t blockIndex = entry.firstBlock;

	while (blockIndex) //First should always go through since there should be no entries with null/root block
	{
		readc = device_read_at_blk(blockIndex, &curBlock, sizeof(BlockHeader), pt);
		ERR_IF(readc != sizeof(BlockHeader), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

		counter += curBlock.usedSpace; //In theory all but last should be full
		blockIndex = curBlock.nextBlock;
	}
	
	*size = counter;

	return DFS_SUCCESS;
}
#pragma endregion
#pragma region File handles
DFS_err handle_can_open(DPartition *pt, const char *path, const DFS_filem_flags flags)
{	return DFS_NOT_IMPLEMENTED;

}
#pragma endregion
