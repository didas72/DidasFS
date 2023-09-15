//DidasFS.c - Implements DidasFS.h

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "DidasFS.h"
#include "DidasFS_structures.h"
#include "DPaths.h"
#include "bitUtils.h"
#include "errUtils.h"



//================================
//= Internal function signatures =
//================================
int DeviceSeek(const int whence, const size_t offset, const DPartition *partition);
size_t DeviceWrite(void *buffer, const size_t len, const DPartition *partition);
size_t DeviceRead(void *buffer, const size_t len, const DPartition *partition);
size_t BlockIndexToAddress(const DPartition *partition, const uint32_t index);
size_t BlkIdxToAddr(const DPartition *partition, const uint32_t index)
{
	return BlockIndexToAddress(partition, index);
}
size_t DeterminePartitionSize(size_t dataSize, size_t *blockCount);
int ForceAllocateSpace(const char *device, size_t size);
int InitEmptyPartition(const char *device, size_t blockCount);
char ValidatePartitionHeader(const DPartition* pt);
int FindEntryPointer(const DPartition* pt, const char *path, EntryPointer *entry);
int FindEntryPointer_Recursion(const DPartition* pt, const uint32_t curBlock, const char *path, EntryPointer *entry);



//============================
//= Function implementations =
//============================
int InitFileSystem(const char *device, size_t dataSize)
{
	int err;
	size_t blockCount;
	size_t size = DeterminePartitionSize(dataSize, &blockCount);

	ERR_NULL(size, DFS_NVAL_ARGS);
	ERR_NZERO((err = ForceAllocateSpace(device, size)), err);

	ERR_NZERO((err = InitEmptyPartition(device, blockCount)), err);

	return DFS_SUCCESS;
}

int OpenFileSystem(const char *device, DPartition **ptHandle)
{
	ERR_NULL(ptHandle, DFS_NVAL_ARGS);

	*ptHandle = NULL;
	int err;

	DPartition* pt = malloc(sizeof(DPartition));
	ERR_NULL(pt, DFS_FAILED_ALLOC);

	pt->device = fopen(device, "r+b");
	ERR_NULL_FREE1(pt->device, DFS_FAILED_DEVICE_OPEN, pt);

	ERR_NZERO_CLEANUP_FREE1((err = ValidatePartitionHeader(pt)), err,
		fclose(pt->device), pt);

	//Determine address of root block for fast access
	uint32_t blockMapSize;
	fseek(pt->device, 4, SEEK_SET);
	size_t read = fread(&blockMapSize, sizeof(uint32_t), 1, pt->device);
	ERR_IF_CLEANUP_FREE1(read != sizeof(uint32_t), DFS_FAILED_DEVICE_READ,
		fclose(pt->device), pt);
	pt->rootBlockAddr = 16 + (size_t)blockMapSize;

	*ptHandle = pt;
	return DFS_SUCCESS;
}

int OpenFile(DPartition *pt, const char *path, DFileStream **fsHandle)
{
	if (!pt)
		return DFS_NVAL_ARGS;
	if (!fsHandle)
		return DFS_NVAL_ARGS;

	*fsHandle = NULL;
	int err;
	EntryPointer entry;

	DFileStream* fs = malloc(sizeof(DFileStream));
	if (!fs)
		return DFS_FAILED_ALLOC;

	ERR_NZERO_FREE1((err = FindEntryPointer(pt, path, &entry)), err, fs);
	
	fs->filePos = 0;
	fs->curBlockIdx = 0;
	fs->firstBlockIdx = entry.firstBlock;
	fs->lastBlockIdx = entry.lastBlock;
	fs->fileFlags = entry.flags;

	*fsHandle = fs;
	return DFS_SUCCESS;
}


//=====================================
//= Internal function implementations =
//=====================================
inline int DeviceSeek(const int whence, const size_t offset, const DPartition *partition)
{
	return fseek(partition->device, offset, whence);
}

inline size_t DeviceWrite(void *buffer, const size_t len, const DPartition *partition)
{
	return fwrite(buffer, len, 1, partition->device);
}

inline size_t DeviceRead(void *buffer, const size_t len, const DPartition *partition)
{
	return fread(buffer, len, 1, partition->device);
}

inline size_t BlockIndexToAddress(const DPartition *partition, const uint32_t index)
{
	return partition->rootBlockAddr + (size_t)index * BLOCK_SIZE;
}

size_t DeterminePartitionSize(size_t dataSize, size_t *blockCount)
{
	if (dataSize <= 0) return 0;
	if (dataSize < BLOCK_SIZE) return 0;
	if (dataSize % BLOCK_SIZE != 0) return 0;

	size_t numBlocks = dataSize / BLOCK_SIZE;
	size_t totalSize = sizeof(PartitionHeader)
		+ (numBlocks >> 3) //block map
		+ numBlocks * BLOCK_SIZE; //data blocks

	if (blockCount)
		*blockCount = numBlocks;

	return totalSize;
}

int ForceAllocateSpace(const char *device, size_t size)
{
	ERR_NULL(device, DFS_NVAL_ARGS);
	ERR_NULL(size, DFS_NVAL_ARGS);
	ERR_IF(size & 0xFFF, DFS_NVAL_ARGS); //size % 4096

	FILE* file = fopen(device, "w+b");

	ERR_NULL(file, DFS_FAILED_DEVICE_OPEN);

	char buff[4096];
	memset(buff, 0, sizeof(buff));
	size_t times = size >> 12; //size / 4096;

	while (times)
	{
		size_t written = fwrite(buff, 1, 4096, file);

		ERR_IF_CLEANUP(written != 4096,
			DFS_FAILED_SPACE_RESERVE, fclose(file));
		times--;
	}

	fclose(file);

	return DFS_SUCCESS;
}

int InitEmptyPartition(const char *device, size_t blockCount)
{
	ERR_NULL(device, DFS_NVAL_ARGS);
	ERR_IF(blockCount == 0, DFS_NVAL_ARGS);

	FILE* file = fopen(device, "r+b");

	ERR_NULL(file, DFS_NVAL_ARGS);

	//Write header
	PartitionHeader header = { .magicNumber = MAGIC_NUMBER,
		.blockMapSize = blockCount >> 3 };

	size_t written = fwrite(&header, sizeof(PartitionHeader), 1, file);

	ERR_IF_CLEANUP(written != sizeof(PartitionHeader),
		DFS_FAILED_DEVICE_WRITE, fclose(file));

	fclose(file);

	return DFS_SUCCESS;
}

char ValidatePartitionHeader(const DPartition* pt)
{
	if (!pt)
		return DFS_NVAL_ARGS;

	uint32_t buff;
	size_t read;
	
	//Check magic number
	read = fread(&buff, sizeof(uint32_t), 1, pt->device);
	ERR_IF(read != sizeof(uint32_t), DFS_FAILED_DEVICE_READ);
	ERR_IF(buff != MAGIC_NUMBER, DFS_CORRUPTED_FS);

	//Check first reserved
	fseek(pt->device, 8, SEEK_SET);
	read = fread(&buff, sizeof(uint32_t), 1, pt->device);
	ERR_IF(read != sizeof(uint32_t), DFS_FAILED_DEVICE_READ);
	ERR_IF(buff != 0, DFS_CORRUPTED_FS);

	//Check second reserved
	read = fread(&buff, sizeof(uint32_t), 1, pt->device);
	ERR_IF(read != sizeof(uint32_t), DFS_FAILED_DEVICE_READ);
	ERR_IF(buff != 0, DFS_CORRUPTED_FS);

	return DFS_SUCCESS;
}

int FindEntryPointer(const DPartition* pt, const char *path, EntryPointer *entry)
{
	//This is the 'intermediate' method
	//Validates arguments and initiates recursion

	ERR_NULL(pt, DFS_NVAL_ARGS);
	ERR_NULL(path, DFS_NVAL_ARGS);
	ERR_NULL(entry, DFS_NVAL_ARGS);

	return FindEntryPointer_Recursion(pt, 0, path, entry);
}

int FindEntryPointer_Recursion(const DPartition* pt, const uint32_t curBlock, const char *path, EntryPointer *entry)
{
	char root[MAX_PATH_NAME + 1];
	char tail[MAX_PATH + 1];
	char *searchName;
	char searchForDir;
	EntryPointer *entries = NULL, foundEntry = { 0 };
	BlockHeader curHeader = { 0 };
	size_t nextIdx = 0;

	DPathGetRoot(root, path);
	DPathGetTail(tail, path);

	if (strlen(root)) //Not recursion base, looking for dir
	{
		searchForDir = ENTRY_FLAG_DIR;
		searchName = root;
	}
	else //Recursion base, looking for file
	{
		searchForDir = 0;
		searchName = tail;
	}

	//Read current block entries
	DeviceSeek(SEEK_SET, BlkIdxToAddr(pt, curBlock), pt); //TODO: Handle errors
	DeviceRead(&curHeader, sizeof(curHeader), pt); //TODO: Handle errors
	DeviceRead(entries, ENTRIES_PER_BLOCK * sizeof(EntryPointer), pt); //TODO: Handle errors

	//TODO: Possibly validate header usedSpace is multiple of sizeof(EntryPointer)
	int validEntryCount = curHeader.usedSpace / sizeof(EntryPointer);

	entries = malloc(ENTRIES_PER_BLOCK * sizeof(EntryPointer));
	ERR_NULL(entries, DFS_FAILED_ALLOC);

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
		break;
	}

	free(entries);

	if (!nextIdx) //Couldn't find dir/file in current block
	{
		if (!curHeader.nextBlock) //And there are no more blocks
			return DFS_PATH_NOT_FOUND;
		else //But there are more blocks
			return FindEntryPointer_Recursion(pt, curHeader.nextBlock, path, entry);
	}
	else //Found dir/file
	{
		if (nextIdx && strlen(root)) //It's not final, continue call stack
			return FindEntryPointer_Recursion(pt, nextIdx, tail, entry);
		else //It's the requested dir/file, return index
		{
			*entry = foundEntry;
			return DFS_SUCCESS;
		}
	}
}
