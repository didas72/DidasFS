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
inline int DeviceSeek(int whence, size_t offset, DPartition *partition);
size_t DeviceWrite(void *buffer, size_t len, DPartition *partition);
size_t DeviceRead(void *buffer, size_t len, DPartition *partition);
size_t DeterminePartitionSize(size_t dataSize, size_t *blockCount);
int ForceAllocateSpace(const char *device, size_t size);
int InitEmptyPartition(const char *device, size_t blockCount);
char ValidatePartitionHeader(DPartition* pt);
int FindFirstBlock(DPartition* pt, const char *path, size_t *address);


//============================
//= Function implementations =
//============================
int InitFileSystem(const char *device, size_t dataSize)
{
	size_t blockCount;
	size_t size = DeterminePartitionSize(dataSize, &blockCount);
	int err = ForceAllocateSpace(device, size);

	if (err)
		return err;

	err = InitEmptyPartition(device, blockCount);

	if (err)
		return err;

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
{return DFS_NOT_IMPLEMENTED;
	if (!pt)
		return DFS_NVAL_ARGS;
	if (!fsHandle)
		return DFS_NVAL_ARGS;

	*fsHandle = NULL;
	int err;

	DFileStream* fs = malloc(sizeof(DFileStream));
	if (!fs)
		return DFS_FAILED_ALLOC;

	//TODO: Find address
	//TODO: Set block address, set pos to 0

	*fsHandle = fs;
	return DFS_SUCCESS;
}


//=====================================
//= Internal function implementations =
//=====================================
inline int DeviceSeek(int whence, size_t offset, DPartition *partition)
{
	return fseek(partition->device, offset, whence);
}

inline size_t DeviceWrite(void *buffer, size_t len, DPartition *partition)
{
	return fwrite(buffer, len, 1, partition->device);
}

inline size_t DeviceRead(void *buffer, size_t len, DPartition *partition)
{
	return fread(buffer, len, 1, partition->device);
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

char ValidatePartitionHeader(DPartition* pt)
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

int FindFirstBlock(DPartition* pt, const char *path, size_t *address)
{return DFS_NOT_IMPLEMENTED;
	ERR_NULL(pt, DFS_NVAL_ARGS);
	ERR_NULL(path, DFS_NVAL_ARGS);
	ERR_NULL(address, DFS_NVAL_ARGS);
	
	size_t curBlock = pt->rootBlockAddr;

	//This is the 'intermediate' method
	//Validates arguments and initiates recursion
	//TODO: Implement recursion base
	//TODO: Implement recursion stepping
}

int FindFirstBlock_Recursion(DPartition* pt, const size_t curBlock, const char *path, size_t *address)
{
	char root[MAX_PATH_NAME + 1];
	char tail[MAX_PATH + 1];
	char *searchName;
	char searchForDir;
	EntryPointer *entries = NULL;
	BlockHeader curHeader = { 0 };

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
	DeviceSeek(SEEK_SET, curBlock, pt); //TODO: Handle errors
	DeviceRead(&curHeader, sizeof(curHeader), pt); //TODO: Handle errors
	DeviceRead(entries, ENTRIES_PER_BLOCK * sizeof(EntryPointer), pt); //TODO: Handle errors

	//TODO: Possibly validate header usedSpace is multiple of sizeof(EntryPointer)
	int validEntryCount = curHeader.usedSpace / sizeof(EntryPointer);

	entries = malloc(ENTRIES_PER_BLOCK * sizeof(EntryPointer));
	ERR_NULL(entries, DFS_FAILED_ALLOC);

	//Search entries for dir/file
	for (int i = 0; i < validEntryCount; i++)
	{
		//First filter out dir/files as needed
		if (entries[i].flags & ENTRY_FLAG_DIR != searchForDir)
			continue;

		
	}

	//TODO: Free allocs
}
