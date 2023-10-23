//didasFS.c - Implements DidasFS.h

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "didasFS.h"
#include "didasFS_structures.h"
#include "dPaths.h"
#include "bitUtils.h"
#include "errUtils.h"


//==================================
//= Internal function declarations =
//==================================
#pragma region Device helpers
int DeviceSeek(const int whence, const size_t offset, const DPartition *partition);
size_t DeviceWrite(void *buffer, const size_t len, const DPartition *partition);
size_t DeviceWriteAt(const size_t address, void *buffer, const size_t len, const DPartition *partition);
size_t DeviceWriteAtBlk(const uint32_t index, void *buffer, const size_t len, const DPartition *partition);
size_t DeviceWriteAtEntryLoc(const EntryPointerLoc entryLoc, EntryPointer *buffer, const DPartition *partition);
size_t DeviceRead(void *buffer, const size_t len, const DPartition *partition);
size_t DeviceReadAt(const size_t address, void *buffer, const size_t len, const DPartition *partition);
size_t DeviceReadAtBlk(const uint32_t index, void *buffer, const size_t len, const DPartition *partition);
size_t DeviceReadAtEntryLoc(const EntryPointerLoc entryLoc, void *buffer, const DPartition *partition);
int ForceAllocateSpace(const char *device, size_t size);
#pragma endregion
#pragma region Block-Address abstraction
size_t BlkIdxToAddr(const DPartition *partition, const uint32_t index);
size_t BlkOffToAddr(const DPartition *partition, const uint32_t index, const size_t offset);
size_t EntryLocToAddr(const DPartition *partition, const EntryPointerLoc entryLoc);
EntryPointerLoc GetRootLoc();
#pragma endregion
#pragma region Code naming
size_t DetermineFirstBlockAddress(uint32_t blockMapSize);
size_t DeterminePartitionSize(size_t dataSize, size_t *blockCount);
int InitEmptyPartition(const char *device, size_t blockCount);
int ValidatePartitionHeader(const DPartition *pt);
int SetStreamPosition(size_t position, DFileStream *fs);
#pragma endregion
#pragma region Block navigation
int FindEntryPointer(const DPartition *pt, const char *path, EntryPointer *entry, EntryPointerLoc *entryLoc);
int FindEntryPointer_Recursion(const DPartition *pt, const uint32_t curBlock, const char *path, EntryPointer *entry, EntryPointerLoc *entryLoc);
int FindFreeBlock(const DPartition *pt, uint32_t *index);
#pragma endregion
#pragma region Block manipulation
int AppendBlockToFile(const DPartition *pt, const EntryPointerLoc entryLoc, uint32_t *newBlkIdx);
int AppendEntryToDir(const DPartition *pt, const EntryPointerLoc dirEntryLoc, EntryPointer newEntry);
#pragma endregion
#pragma region File manipulation
int CreateObject(DPartition *pt, const char *path, const uint16_t flags);

int DetermineFileSize(DPartition *pt, const EntryPointer entry, size_t *size);
#pragma endregion


//============================
//= Function implementations =
//============================
#pragma region Function implementations
int InitPartition(const char *device, size_t dataSize)
{
	int err;
	size_t blockCount;
	size_t size = DeterminePartitionSize(dataSize, &blockCount);

	ERR_NULL(size, DFS_NVAL_ARGS, "Invalid data size %ld.\n", dataSize);
	ERR_NZERO((err = ForceAllocateSpace(device, size)), err, "Failed to allocate space.\n");

	ERR_NZERO((err = InitEmptyPartition(device, blockCount)), err, "Failed to init empty partition.\n");

	return DFS_SUCCESS;
}

int OpenPartition(const char *device, DPartition **ptHandle)
{
	ERR_NULL(ptHandle, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(ptHandle));

	*ptHandle = NULL;
	int err;

	DPartition* pt = malloc(sizeof(DPartition));
	ERR_NULL(pt, DFS_FAILED_ALLOC, ERR_MSG_ALLOC_FAIL);

	pt->device = fopen(device, "r+b");
	ERR_NULL_FREE1(pt->device, DFS_FAILED_DEVICE_OPEN, pt, "Failed to open device %s.\n", device);

	ERR_NZERO_CLEANUP_FREE1((err = ValidatePartitionHeader(pt)), err,
		fclose(pt->device), pt, "Partition header validation failed.\n");

	//Determine address of root block for fast access
	uint32_t blockMapSize;
	fseek(pt->device, 4, SEEK_SET);
	size_t read = fread(&blockMapSize, sizeof(uint32_t), 1, pt->device);
	ERR_IF_CLEANUP_FREE1(read != 1, DFS_FAILED_DEVICE_READ,
		fclose(pt->device), pt, ERR_MSG_DEVICE_READ_FAIL);

	pt->rootBlockAddr = DetermineFirstBlockAddress(blockMapSize);
	pt->blockCount = blockMapSize << 3;

	err = LoadBlockMap(pt);

	ERR_NZERO_CLEANUP_FREE1(err, err, fclose(pt->device), pt, "Failed to load block map.\n");

	*ptHandle = pt;
	return DFS_SUCCESS;
}

int ClosePartition(DPartition *ptHandle)
{
	ERR_NULL(ptHandle, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(ptHandle));

	int err;

	ERR_NZERO((err = FlushFullBlockMap(ptHandle)), err, "Failed to flush block map.\n");
	ERR_NZERO((err = DestroyBlockMap(ptHandle)), err, "Failed to destroy block map.\n");
	fclose(ptHandle->device);
	free(ptHandle);

	return DFS_SUCCESS;
}

int CreateDirectory(DPartition *pt, const char *path)
{
	return CreateObject(pt, path, ENTRY_FLAG_DIR | ENTRY_FLAG_READWRITE);
}

int CreateFile(DPartition *pt, const char *path)
{
	return CreateObject(pt, path, ENTRY_FLAG_FILE | ENTRY_FLAG_READWRITE);
}

int OpenFile(DPartition *pt, const char *path, DFileStream **fsHandle)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(fsHandle, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(fsHandle));

	*fsHandle = NULL;
	int err;
	EntryPointer entry;
	EntryPointerLoc entryLoc;

	DFileStream* fs = malloc(sizeof(DFileStream));
	ERR_NULL(fs, DFS_FAILED_ALLOC, ERR_MSG_ALLOC_FAIL);

	ERR_NZERO_FREE1((err = FindEntryPointer(pt, path, &entry, &entryLoc)), err, fs, "Could not find entry for the requested file.\n");
	
	ERR_NZERO_FREE1((err = DetermineFileSize(pt, entry, &fs->fileSize)), err, fs, "Failed to determine file size.\n");
	fs->filePos = 0;
	fs->curBlockIdx = entry.firstBlock;
	fs->firstBlockIdx = entry.firstBlock;
	fs->lastBlockIdx = entry.lastBlock;
	//fs->fileFlags = entry.flags;
	fs->entryLoc = entryLoc;
	fs->pt = pt;

	*fsHandle = fs;
	return DFS_SUCCESS;
}

int CloseFile(DFileStream *fs)
{
	ERR_NULL(fs, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(fs));

	//Handle pt stuff with fs->pt

	free(fs);
	
	return DFS_SUCCESS;
}

int FileWrite(void *buffer, size_t len, DFileStream *fs, size_t *written)
{
	ERR_NULL(buffer, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(buffer));
	ERR_NULL(fs, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(fs));
	ERR_IF(len == 0, DFS_NVAL_ARGS, "Argument 'len' must not be 0.\n");

	size_t bufferHead = 0, read;
	BlockHeader curBlock;
	int err;
	
	//Should work for both append and overwrite
	while (bufferHead < len)
	{
		//Read cur block
		read = DeviceReadAtBlk(fs->curBlockIdx, &curBlock, sizeof(BlockHeader), fs->pt);
		ERR_IF(read != sizeof(BlockHeader), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL); //TODO: Revert changes?

		//Calculate metrics
		size_t blockOffset = fs->filePos % BLOCK_DATA_SIZE;
		size_t dataAddr = BlkOffToAddr(fs->pt, fs->curBlockIdx, blockOffset);
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
			read = DeviceWriteAtBlk(fs->curBlockIdx, &curBlock, sizeof(BlockHeader), fs->pt);
			ERR_IF(read != sizeof(BlockHeader), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL); //TODO: Revert changes?
		}

		//Flush data
		read = DeviceWriteAt(dataAddr, &((char*)buffer)[bufferHead], curBlockWrite, fs->pt);
		ERR_NZERO(read != curBlockWrite, DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL); //TODO: Revert changes?

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
				err = AppendBlockToFile(fs->pt, fs->entryLoc, &newIndex);
				ERR_NZERO(err, err, "Failed to grow file.\n");
				fs->curBlockIdx = fs->lastBlockIdx = newIndex;
			}
		}
	}

	if (written)
		*written = bufferHead;

	return DFS_SUCCESS;
}

int FileRead(void *buffer, size_t len, DFileStream *fs, size_t *read)
{
	ERR_NULL(buffer, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(buffer));
	ERR_NULL(fs, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(fs));
	ERR_IF(len == 0, DFS_NVAL_ARGS, "Argument 'len' must not be 0.\n");

	size_t bufferHead = 0, readB;
	BlockHeader curBlock;

	while (bufferHead < len)
	{
		//Read cur block
		readB = DeviceReadAtBlk(fs->curBlockIdx, &curBlock, sizeof(BlockHeader), fs->pt);
		ERR_IF(readB != sizeof(BlockHeader), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL); //TODO: Revert changes?

		//Calculate metrics
		size_t blockOffset = fs->filePos % BLOCK_DATA_SIZE;
		size_t dataAddr = BlkOffToAddr(fs->pt, fs->curBlockIdx, blockOffset);
		size_t maxRead = BLOCK_DATA_SIZE - blockOffset;
		size_t pendingRead = len - bufferHead;
		size_t curBlockRead = (maxRead < pendingRead) ? maxRead : pendingRead;
		
		//Read data
		readB = DeviceReadAt(dataAddr, &((char*)buffer)[bufferHead], curBlockRead, fs->pt);
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
				if (read)
					*read = bufferHead;
				return DFS_SUCCESS;
			}
		}
	}

	if (read)
		*read = bufferHead;

	return DFS_SUCCESS;
}

int FileSetPos(size_t pos, DFileStream *fs)
{
	ERR_NULL(fs, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(fs));

	//TODO: Consider 'inlining'
	return SetStreamPosition(pos, fs);
}
#pragma endregion


//======================================
//= _structures method implementations =
//======================================
#pragma region _structures method implementations
int LoadBlockMap(DPartition* host)
{
	ERR_NULL(host, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(host));

	BlockMap *map = malloc(sizeof(BlockMap));

	ERR_NULL(map, DFS_FAILED_ALLOC, ERR_MSG_ALLOC_FAIL);

	map->host = host;
	map->length = host->blockCount >> 3;
	map->map = malloc(map->length);

	ERR_NULL_FREE1(map->map, DFS_FAILED_ALLOC, map, ERR_MSG_ALLOC_FAIL);

	size_t read = DeviceReadAt(sizeof(PartitionHeader), map->map, map->length, host);

	ERR_IF_FREE2(read != map->length, DFS_FAILED_DEVICE_READ, map->map, map, ERR_MSG_DEVICE_READ_FAIL);

	host->blockMap = map;

	return DFS_SUCCESS;
}

int GetBlockUsed(const DPartition *pt, uint32_t blockIndex, bool *used)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(used, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(used));
	ERR_IF(blockIndex >= pt->blockCount, DFS_NVAL_ARGS, "Argument 'blockIndex' must be smaller than total block count.\n");

	uint32_t offset = blockIndex & 0x7; 
	uint32_t index = blockIndex & ~0x7;

	*used = pt->blockMap->map[index] & (1 << offset);

	return DFS_SUCCESS;
}

int SetBlockUsed(const DPartition *pt, uint32_t blockIndex, bool used)
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

int FlushFullBlockMap(const DPartition *pt)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));

	DeviceSeek(SEEK_SET, sizeof(PartitionHeader), pt);
	size_t written = DeviceWrite(pt->blockMap->map, pt->blockMap->length, pt);

	ERR_IF(written != pt->blockMap->length, DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);

	return DFS_SUCCESS;
}

int DestroyBlockMap(DPartition *pt)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));

	free(pt->blockMap->map);
	free(pt->blockMap);

	return DFS_SUCCESS;
}
#pragma endregion


//=====================================
//= Internal function implementations =
//=====================================
#pragma region Device helpers
inline int DeviceSeek(const int whence, const size_t offset, const DPartition *partition)
{
	return fseek(partition->device, offset, whence);
}

inline size_t DeviceWrite(void *buffer, const size_t len, const DPartition *partition)
{
	return fwrite(buffer, 1, len, partition->device);
}
inline size_t DeviceWriteAt(const size_t address, void *buffer, const size_t len, const DPartition *partition)
{
	DeviceSeek(SEEK_SET, address, partition);
	return DeviceWrite(buffer, len, partition);
}
inline size_t DeviceWriteAtBlk(const uint32_t index, void *buffer, const size_t len, const DPartition *partition)
{
	size_t addr = BlkIdxToAddr(partition, index);
	DeviceSeek(SEEK_SET, addr, partition);
	return DeviceWrite(buffer, len, partition);
}
inline size_t DeviceWriteAtEntryLoc(const EntryPointerLoc entryLoc, EntryPointer *buffer, const DPartition *partition)
{
	size_t addr = EntryLocToAddr(partition, entryLoc);
	DeviceSeek(SEEK_SET, addr, partition);
	return DeviceWrite(buffer, sizeof(EntryPointer), partition);
}

inline size_t DeviceRead(void *buffer, const size_t len, const DPartition *partition)
{
	return fread(buffer, 1, len, partition->device);
}
inline size_t DeviceReadAt(const size_t address, void *buffer, const size_t len, const DPartition *partition)
{
	DeviceSeek(SEEK_SET, address, partition);
	return DeviceRead(buffer, len, partition);
}
inline size_t DeviceReadAtBlk(const uint32_t index, void *buffer, const size_t len, const DPartition *partition)
{
	size_t addr = BlkIdxToAddr(partition, index);
	DeviceSeek(SEEK_SET, addr, partition);
	return DeviceRead(buffer, len, partition);
}
inline size_t DeviceReadAtEntryLoc(const EntryPointerLoc entryLoc, void *buffer, const DPartition *partition)
{
	size_t addr = EntryLocToAddr(partition, entryLoc);
	DeviceSeek(SEEK_SET, addr, partition);
	return DeviceRead(buffer, sizeof(EntryPointer), partition);
}

int ForceAllocateSpace(const char *device, size_t size)
{
	ERR_NULL(device, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(device));
	ERR_IF(size == 0, DFS_NVAL_ARGS, "Argument 'size' must not be 0.");

	FILE* file = fopen(device, "w+b");
	char zero = 0;

	ERR_NULL(file, DFS_FAILED_DEVICE_OPEN, ERR_MSG_DEVICE_OPEN_FAIL);

	fseek(file, size - 1, SEEK_SET);
	fwrite(&zero, 1, 1, file);

	fclose(file);

	return DFS_SUCCESS;
}
#pragma endregion
#pragma region Block-Address abstraction
size_t BlkIdxToAddr(const DPartition *partition, const uint32_t index)
{
	return partition->rootBlockAddr + (size_t)index * BLOCK_SIZE;
}
size_t BlkOffToAddr(const DPartition *partition, const uint32_t index, const size_t offset)
{
	return BlkIdxToAddr(partition, index) + sizeof(BlockHeader) + offset;
}
size_t EntryLocToAddr(const DPartition *partition, const EntryPointerLoc entryLoc)
{
	if (entryLoc.blockIndex == ~0u && entryLoc.entryIndex == ~0u)
		return partition->rootBlockAddr - sizeof(EntryPointer);

	size_t baseAddress = BlkIdxToAddr(partition, entryLoc.blockIndex);
	size_t offset = sizeof(BlockHeader) + sizeof(EntryPointer) * entryLoc.entryIndex;

	return baseAddress + offset;
}
EntryPointerLoc GetRootLoc()
{
	EntryPointerLoc loc = { .blockIndex = ~0u, .entryIndex = ~0u };
	return loc;
}
#pragma endregion
#pragma region Code naming
size_t DetermineFirstBlockAddress(uint32_t blockMapSize)
{
	size_t withoutPad = (size_t)blockMapSize + sizeof(PartitionHeader) + sizeof(EntryPointer);
	size_t rem = withoutPad & (SECTOR_SIZE - 1);

	return withoutPad + (rem ? SECTOR_SIZE - rem : 0);
}

size_t DeterminePartitionSize(size_t dataSize, size_t *blockCount)
{
	if (dataSize <= 0) return 0;
	if (dataSize < BLOCK_SIZE) return 0;
	if (dataSize % BLOCK_SIZE != 0) return 0;

	size_t numBlocks = dataSize / BLOCK_SIZE;
	size_t blockMapSize = numBlocks >> 3;

	//DTS = DaTa Start
	size_t DTS = DetermineFirstBlockAddress(blockMapSize);

	size_t totalSize = DTS + numBlocks * BLOCK_SIZE;

	if (blockCount)
		*blockCount = numBlocks;

	return totalSize;
}

int InitEmptyPartition(const char *device, size_t blockCount)
{
	ERR_NULL(device, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(device));
	ERR_IF(blockCount == 0, DFS_NVAL_ARGS, "Argument 'blockCount' must not be 0.\n");

	FILE* file = fopen(device, "r+b");

	ERR_NULL(file, DFS_NVAL_ARGS, ERR_MSG_DEVICE_OPEN_FAIL);

	//Write header
	PartitionHeader header = { .magicNumber = MAGIC_NUMBER,
		.blockMapSize = blockCount >> 3 };

	size_t written = fwrite(&header, sizeof(PartitionHeader), 1, file);

	ERR_IF_CLEANUP(written != 1,
		DFS_FAILED_DEVICE_WRITE, fclose(file), ERR_MSG_DEVICE_WRITE_FAIL);

	char rootUsed = 1;
	written = fwrite(&rootUsed, 1, 1, file);
	ERR_IF_CLEANUP(written != 1,
		DFS_FAILED_DEVICE_WRITE, fclose(file), ERR_MSG_DEVICE_WRITE_FAIL);


	fclose(file);

	return DFS_SUCCESS;
}

int ValidatePartitionHeader(const DPartition* pt)
{
	if (!pt)
		return DFS_NVAL_ARGS;

	PartitionHeader buff;
	size_t read;
	
	read = fread(&buff, sizeof(PartitionHeader), 1, pt->device);
	ERR_IF(read != 1, DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

	//Check magic number
	ERR_IF(buff.magicNumber != MAGIC_NUMBER, DFS_CORRUPTED_FS, "Partition has incorrect magic number.\n");

	//Check reserved
	ERR_IF(buff.resvd != 0, DFS_CORRUPTED_FS, "Partition has reserved flags set.\n");

	//Check blockCount will not overflow
	ERR_IF(buff.blockMapSize > (~0u >> 3), DFS_CORRUPTED_FS, "Partition blockMapSize is too big.\n");

	return DFS_SUCCESS;
}

int SetStreamPosition(size_t position, DFileStream *fs)
{
	ERR_IF(position > fs->fileSize, DFS_NVAL_SEEK, "Attempted to seek beyond file boundaries.");

	size_t curPos = 0, left;
	uint32_t curBlock = fs->firstBlockIdx;
	BlockHeader curHeader;
	size_t read;

	while (curPos != position)
	{
		left = curPos - position;
		read = DeviceReadAtBlk(curBlock, &curHeader, sizeof(BlockHeader), fs->pt);
		ERR_IF(read != sizeof(BlockHeader), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

		if (left >= BLOCK_DATA_SIZE) //Full block skip
		{
			if (curHeader.nextBlock == 0) //Last block in file
				return DFS_NVAL_SEEK;

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
int FindEntryPointer(const DPartition* pt, const char *path, EntryPointer *entry, EntryPointerLoc *entryLoc)
{
	//This is the 'intermediate' method
	//Validates arguments and initiates recursion

	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(path, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(path));

	if (strlen(path) == 0)
	{
		EntryPointerLoc loc = GetRootLoc();

		if (entry)
		{
			EntryPointer e;
			size_t read = DeviceReadAtEntryLoc(loc, &e, pt);
			ERR_IF(read != sizeof(EntryPointer), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);
			*entry = e;
		}

		if (entryLoc)
			*entryLoc = loc;

		return DFS_SUCCESS;
	}

	return FindEntryPointer_Recursion(pt, 0, path, entry, entryLoc);
}

int FindEntryPointer_Recursion(const DPartition* pt, const uint32_t curBlock, const char *path, EntryPointer *entry, EntryPointerLoc *entryLoc)
{
	char root[MAX_PATH_NAME + 1];
	char tail[MAX_PATH + 1];
	char *searchName;
	char searchForDir;
	EntryPointer *entries = NULL, foundEntry = { 0 };
	EntryPointerLoc location = { 0 };
	BlockHeader curHeader = { 0 };
	uint32_t nextIdx = 0;
	size_t read;

	memset(root, 0, MAX_PATH_NAME + 1);
	memset(tail, 0, MAX_PATH + 1);

	DPathGetRoot(root, path);
	DPathGetTail(tail, path);

	if (strlen(root)) //Looking for dir
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
	DeviceSeek(SEEK_SET, BlkIdxToAddr(pt, curBlock), pt); //TODO: Handle errors
	read = DeviceRead(&curHeader, sizeof(BlockHeader), pt);
	ERR_IF(read != sizeof(BlockHeader), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL)

	entries = malloc(ENTRIES_PER_BLOCK * sizeof(EntryPointer));
	ERR_NULL(entries, DFS_FAILED_ALLOC, ERR_MSG_ALLOC_FAIL);

	read = DeviceRead(entries, ENTRIES_PER_BLOCK * sizeof(EntryPointer), pt); //TODO: Handle errors
	ERR_IF(read != ENTRIES_PER_BLOCK * sizeof(EntryPointer), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL)

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
		if (!curHeader.nextBlock) //And there are no more blocks
			return DFS_PATH_NOT_FOUND;
		else //But there are more blocks
			return FindEntryPointer_Recursion(pt, curHeader.nextBlock, path, entry, entryLoc);
	}
	else //Found dir/file
	{
		if (nextIdx && strlen(root)) //It's not final, continue search on next block
			return FindEntryPointer_Recursion(pt, nextIdx, tail, entry, entryLoc);
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

int FindFreeBlock(const DPartition *pt, uint32_t *index)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(index, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(index));

	int err;
	uint32_t i;

	//TODO: Optimize to use blockMap byte searches (8 bits at a time)
	for (i = 0; i < pt->blockCount; i++)
	{
		bool used;
		ERR_NZERO((err = GetBlockUsed(pt, i, &used)), err, "Failed to retrieve block usage state.\n");

		if (!used)
		{
			*index = i;
			return DFS_SUCCESS;
		}
	}

	return DFS_NO_SPACE;
}
#pragma endregion
#pragma region Block manipulation
int AppendBlockToFile(const DPartition *pt, const EntryPointerLoc entryLoc, uint32_t *newBlkIdx)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));

	int err; size_t read;
	uint32_t newBlockIndex, oldBlockIndex;
	EntryPointer entry;
	BlockHeader newBlock, oldBlock;

	//Find block and reserve
	ERR_NZERO((err = FindFreeBlock(pt, &newBlockIndex)), err, "Could not find a free block.\n");
	ERR_NZERO((err = SetBlockUsed(pt, newBlockIndex, true)), err, "Coult not flag block as used.\n");
	ERR_NZERO((err = FlushFullBlockMap(pt)), err, "Failed to flush block map.\n");

	//Read entry pointer
	read = DeviceReadAtEntryLoc(entryLoc, &entry, pt);
	ERR_IF(read != sizeof(EntryPointer), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL); //TODO: Revert block set to used

	//Update last block index in entry
	oldBlockIndex = entry.lastBlock;
	entry.lastBlock = newBlockIndex;

	//Flush changes
	read = DeviceWriteAtEntryLoc(entryLoc, &entry, pt);
	ERR_IF(read != sizeof(EntryPointer), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL); //TODO: Revert block set to used

	//Create new block header
	newBlock.nextBlock = 0;
	newBlock.prevBlock = oldBlockIndex;
	newBlock.usedSpace = 0;
	newBlock.resvd = 0;

	//Store new block
	read = DeviceWriteAtBlk(newBlockIndex, &newBlock, sizeof(BlockHeader), pt);
	ERR_IF(read != sizeof(BlockHeader), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);

	//Read old block
	read = DeviceReadAtBlk(oldBlockIndex, &oldBlock, sizeof(BlockHeader), pt);
	ERR_IF(read != sizeof(BlockHeader), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

	//Update index
	oldBlock.nextBlock = newBlockIndex;

	//Flush changes
	read = DeviceWriteAtBlk(oldBlockIndex, &oldBlock, sizeof(BlockHeader), pt);
	ERR_IF(read != sizeof(BlockHeader), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);

	if (newBlkIdx)
		*newBlkIdx = newBlockIndex;

	return DFS_SUCCESS;
}

int AppendEntryToDir(const DPartition *pt, const EntryPointerLoc dirEntryLoc, EntryPointer newEntry)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));

	EntryPointer dirEntry;
	BlockHeader dirBlock;
	uint32_t blockIndex;
	int err; size_t read;

	//Read dir entry
	read = DeviceReadAtEntryLoc(dirEntryLoc, &dirEntry, pt);
	ERR_IF(read != sizeof(EntryPointer), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

	//Load last block
	blockIndex = dirEntry.lastBlock;
	read = DeviceReadAtBlk(blockIndex, &dirBlock, sizeof(BlockHeader), pt);
	ERR_IF(read != sizeof(BlockHeader), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

	if (dirBlock.usedSpace + sizeof(EntryPointer) >= BLOCK_DATA_SIZE) //No free space
	{
		//Append block and update block index
		ERR_NZERO((err = AppendBlockToFile(pt, dirEntryLoc, &blockIndex)), err, "Failed to append block to file.\n");

		//Load new block
		read = DeviceReadAtBlk(blockIndex, &dirBlock, sizeof(BlockHeader), pt);
		ERR_IF(read != sizeof(BlockHeader), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL); //TODO: Revert changes
	}

	//Write new entry pointer
	size_t addr = BlkOffToAddr(pt, blockIndex, dirBlock.usedSpace);
	read = DeviceWriteAt(addr, &newEntry, sizeof(EntryPointer), pt);
	ERR_IF(read != sizeof(EntryPointer), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL); //TODO: Revert changes

	//Update block header
	dirBlock.usedSpace += (uint32_t)sizeof(EntryPointer);

	//Flush header changes
	read = DeviceWriteAtBlk(blockIndex, &dirBlock, sizeof(BlockHeader), pt);
	ERR_IF(read != sizeof(BlockHeader), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL); //TODO: Revert changes

	return DFS_SUCCESS;
}
#pragma endregion
#pragma region File manipulation
int CreateObject(DPartition *pt, const char *path, const uint16_t flags)
{
	char parentDir[MAX_PATH + 1];
	char name[MAX_PATH_NAME + 1];

	int err;
	EntryPointer newEntry = { 0 };
	EntryPointerLoc parentLoc;
	BlockHeader newBlock;
	uint32_t newBlockIndex;
	size_t read;

	DPathGetParent(parentDir, path);
	DPathGetName(name, path);

	//Find parent dir
	ERR_NZERO((err = FindEntryPointer(pt, parentDir, NULL, &parentLoc)), err, "Could not find entry for the requested file.\n");

	//Find and reserve free block
	ERR_NZERO((err = FindFreeBlock(pt, &newBlockIndex)), err, "Could not find free block.\n");
	ERR_NZERO((err = SetBlockUsed(pt, newBlockIndex, true)), err, "Could not flag block as used.\n");
	ERR_NZERO((err = FlushFullBlockMap(pt)), err, "Could not flush block map.\n");

	//Create new entry
	memset(newEntry.name, 0, MAX_PATH_NAME);
	strncpy(newEntry.name, name, MAX_PATH_NAME);
	newEntry.firstBlock = newBlockIndex;
	newEntry.lastBlock = newBlockIndex;
	newEntry.resvd = 0;
	newEntry.flags = flags;

	//Append entry to parent
	ERR_NZERO((err = AppendEntryToDir(pt, parentLoc, newEntry)), err, "Could not append entry to directory.\n");

	//Set new block header
	newBlock.nextBlock = 0;
	newBlock.prevBlock = 0;
	newBlock.usedSpace = 0;
	newBlock.resvd = 0;

	//Flush changes
	read = DeviceWriteAtBlk(newBlockIndex, &newBlock, sizeof(BlockHeader), pt);
	ERR_IF(read != sizeof(BlockHeader), DFS_FAILED_DEVICE_WRITE, ERR_MSG_DEVICE_WRITE_FAIL);

	return DFS_SUCCESS;
}

int DetermineFileSize(DPartition *pt, const EntryPointer entry, size_t *size)
{
	ERR_NULL(pt, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(pt));
	ERR_NULL(size, DFS_NVAL_ARGS, ERR_MSG_NULL_ARG(size));

	size_t counter = 0, read;
	BlockHeader curBlock;
	uint32_t blockIndex = entry.firstBlock;

	while (blockIndex) //First should always go through since there should be no entries with null/root block
	{
		read = DeviceReadAtBlk(blockIndex, &curBlock, sizeof(BlockHeader), pt);
		ERR_IF(read != sizeof(BlockHeader), DFS_FAILED_DEVICE_READ, ERR_MSG_DEVICE_READ_FAIL);

		counter += curBlock.usedSpace; //In theory all but last should be full
		blockIndex = curBlock.nextBlock;
	}
	
	*size = counter;

	return DFS_SUCCESS;
}
#pragma endregion