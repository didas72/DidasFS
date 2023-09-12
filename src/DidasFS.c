//DidasFS.c - Implements DidasFS.h

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "DidasFS.h"
#include "DidasFS_structures.h"
#include "bitUtils.h"



//================================
//= Internal function signatures =
//================================
static size_t DeterminePartitionSize(size_t dataSize, size_t *blockCount);
static int ForceAllocateSpace(char *device, size_t size);
static int InitEmptyPartition(char *device, size_t blockCount);
static char ValidatePartitionHeader(DPartition* pt);


//============================
//= Function implementations =
//============================
int InitFileSystem(char *device, size_t dataSize)
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

int OpenFileSystem(char *device, DPartition **ptHandle)
{
	if (!ptHandle)
		return DFS_NVAL_ARGS;

	*ptHandle = NULL;
	int err;

	DPartition* pt = malloc(sizeof(DPartition));
	if (!pt)
		return DFS_FAILED_ALLOC;

	pt->device = fopen(device, "r+b");
	if (!pt->device)
		return DFS_FAILED_DEVICE_OPEN; //TODO: Dealloc

	if ((err = ValidatePartitionHeader(pt)))
		return err; //TODO: Dealloc and close

	//Determine address of root block for fast access
	uint32_t blockMapSize;
	fseek(pt->device, 4, SEEK_SET);
	size_t read = fread(&blockMapSize, sizeof(uint32_t), 1, pt->device);
	if (read != sizeof(uint32_t))
		return DFS_FAILED_DEVICE_READ; //TODO: Dealloc and close
	pt->rootBlockAddr = 16 + (size_t)blockMapSize;

	*ptHandle = pt;
	return DFS_SUCCESS;
}



//=====================================
//= Internal function implementations =
//=====================================
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

int ForceAllocateSpace(char *device, size_t size)
{
	if (size % 4096 != 0)
		return DFS_NVAL_ARGS;

	FILE* file = fopen(device, "w+b");

	if (file == NULL)
		return DFS_FAILED_DEVICE_OPEN;

	char buff[4096];
	memset(buff, 0, sizeof(buff));
	size_t times = size / 4096;

	while (times)
	{
		size_t written = fwrite(buff, 1, 4096, file);

		if (written != 4096)
		{
			fclose(file);
			return DFS_FAILED_SPACE_RESERVE;
		}
		times--;
	}

	fclose(file);

	return DFS_SUCCESS;
}

int InitEmptyPartition(char *device,  size_t blockCount)
{
	FILE* file = fopen(device, "r+b");

	if (file == NULL)
		return DFS_FAILED_DEVICE_OPEN;


	//Write header
	PartitionHeader header = { .magicNumber = MAGIC_NUMBER,
	.blockMapSize = blockCount >> 3 };

	size_t written = fwrite(&header, sizeof(PartitionHeader), 1, file);

	if (written != sizeof(PartitionHeader))
	{
		fclose(file);
		return DFS_FAILED_DEVICE_WRITE;
	}

	fclose(file);

	return DFS_SUCCESS;
}

char ValidatePartitionHeader(DPartition* pt)
{
	uint32_t buff;
	size_t read;
	
	//Check magic number
	read = fread(&buff, sizeof(uint32_t), 1, pt->device);
	if (read != sizeof(uint32_t))
		return DFS_FAILED_DEVICE_READ;
	if (buff != MAGIC_NUMBER)
		return DFS_CORRUPTED_FS;

	//Check first reserved
	fseek(pt->device, 8, SEEK_SET);
	read = fread(&buff, sizeof(uint32_t), 1, pt->device);
	if (read != sizeof(uint32_t))
		return DFS_FAILED_DEVICE_READ;
	if (buff != 0)
		return DFS_NVAL_FLAGS;

	//Check second reserved
	read = fread(&buff, sizeof(uint32_t), 1, pt->device);
	if (read != sizeof(uint32_t))
		return DFS_FAILED_DEVICE_READ;
	if (buff != 0)
		return DFS_NVAL_FLAGS;

	return DFS_SUCCESS;
}
