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


//==================================
//= Internal function declarations =
//==================================
#pragma region Device helpers
int DeviceSeek(const int whence, const size_t offset, const DPartition *partition);
size_t DeviceWrite(void *buffer, const size_t len, const DPartition *partition);
size_t DeviceWriteAt(const size_t address, void *buffer, const size_t len, const DPartition *partition);
size_t DeviceWriteAtBlk(const uint32_t index, void *buffer, const size_t len, const DPartition *partition);
size_t DeviceWriteAtEntryLoc(const EntryPointerLoc entryLoc, void *buffer, const DPartition *partition);
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
#pragma endregion
#pragma region Code naming
size_t DetermineFirstBlockAddress(uint32_t blockMapSize);
size_t DeterminePartitionSize(size_t dataSize, size_t *blockCount);
int InitEmptyPartition(const char *device, size_t blockCount);
int ValidatePartitionHeader(const DPartition *pt);
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

	pt->rootBlockAddr = DetermineFirstBlockAddress(blockMapSize);
	pt->blockCount = blockMapSize << 3;

	err = LoadBlockMap(pt);

	ERR_NZERO_CLEANUP_FREE1(err, err, fclose(pt->device), pt);

	*ptHandle = pt;
	return DFS_SUCCESS;
}

int CloseFileSystem(DPartition *ptHandle)
{
	ERR_NULL(ptHandle, DFS_NVAL_ARGS);

	int err;

	ERR_NZERO((err = FlushFullBlockMap(ptHandle)), err);
	ERR_NZERO((err = DestroyBlockMap(ptHandle)), err);
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
	ERR_NULL(pt, DFS_NVAL_ARGS);
	ERR_NULL(fsHandle, DFS_NVAL_ARGS);

	*fsHandle = NULL;
	int err;
	EntryPointer entry;
	EntryPointerLoc entryLoc;

	DFileStream* fs = malloc(sizeof(DFileStream));
	ERR_NULL(fs, DFS_FAILED_ALLOC);

	ERR_NZERO_FREE1((err = FindEntryPointer(pt, path, &entry, &entryLoc)), err, fs);
	
	ERR_NZERO_FREE1((err = DetermineFileSize(pt, entry, &fs->fileSize)), err, fs);
	fs->filePos = 0;
	fs->curBlockIdx = 0;
	fs->firstBlockIdx = entry.firstBlock;
	fs->lastBlockIdx = entry.lastBlock;
	//fs->fileFlags = entry.flags;
	fs->entryLoc = entryLoc;
	fs->pt = pt;

	*fsHandle = fs;
	return DFS_SUCCESS;
}

int CloseFile(DPartition *pt, DFileStream *fs)
{
	ERR_NULL(pt, DFS_NVAL_ARGS);
	ERR_NULL(fs, DFS_NVAL_ARGS);

	free(fs);
	
	return DFS_SUCCESS;
}

//TODO: TEST TEST TEST. ALL CASES. Interrupted half way through and just spedrun finishing it
int FileWrite(void *buffer, size_t len, DFileStream *fs, size_t *written)
{
	ERR_NULL(buffer, DFS_NVAL_ARGS);
	ERR_NULL(fs, DFS_NVAL_ARGS);
	ERR_IF(len == 0, DFS_NVAL_ARGS);

	size_t bufferHead = 0;
	BlockHeader curBlock;
	int err;
	
	//Should work for both append and overwrite
	while (bufferHead < len)
	{
		//Read cur block
		err = DeviceReadAtBlk(fs->curBlockIdx, &curBlock, sizeof(BlockHeader), fs->pt);
		ERR_NZERO(err, err); //TODO: Revert changes?

		//Calculate metrics
		size_t blockOffset = fs->filePos % BLOCK_DATA_SIZE;
		size_t dataAddr = BlkOffToAddr(fs->pt, fs->curBlockIdx, blockOffset);
		size_t curBlockMaxWrite = BLOCK_DATA_SIZE - blockOffset;
		size_t pendingWrite = len - bufferHead;
		size_t curBlockWrite = (curBlockMaxWrite < pendingWrite) ? curBlockMaxWrite : pendingWrite;
		size_t finalNewDataInBlock = blockOffset + curBlockWrite;
		size_t finalDataInBlock = (curBlock.usedSpace > finalNewDataInBlock) ? curBlock.usedSpace : finalNewDataInBlock;

		if (finalNewDataInBlock > curBlock.usedSpace) //If grown
		{
			fs->fileSize += finalNewDataInBlock - curBlock.usedSpace;

			//Update cur block
			curBlock.usedSpace = finalDataInBlock;

			//Flush block changes
			err = DeviceWriteAtBlk(fs->curBlockIdx, &curBlock, sizeof(BlockHeader), fs->pt);
			ERR_NZERO(err, err); //TODO: Revert changes?
		}

		//Flush data
		err = DeviceWriteAt(dataAddr, &((char*)buffer)[bufferHead], curBlockWrite, fs->pt);
		ERR_NZERO(err, err); //TODO: Revert changes?

		//Update head
		bufferHead += curBlockWrite;
	}

	if (written)
		*written = bufferHead;

	return DFS_SUCCESS;
}
#pragma endregion


//======================================
//= _structures method implementations =
//======================================
#pragma region _structures method implementations
int LoadBlockMap(DPartition* host)
{
	ERR_NULL(host, DFS_NVAL_ARGS);

	BlockMap *map = malloc(sizeof(BlockMap));

	ERR_NULL(map, DFS_FAILED_ALLOC);

	map->host = host;
	map->length = host->blockCount >> 3;
	map->map = malloc(map->length);

	ERR_NULL_FREE1(map->map, DFS_FAILED_ALLOC, map);

	size_t read = DeviceRead(map->map, map->length, host);

	ERR_IF_FREE2(read != map->length, DFS_FAILED_DEVICE_READ, map->map, map);

	host->blockMap = map;

	return DFS_SUCCESS;
}

int GetBlockUsed(const DPartition *pt, uint32_t blockIndex, bool *used)
{
	ERR_NULL(pt, DFS_NVAL_ARGS);
	ERR_NULL(used, DFS_NVAL_ARGS);
	ERR_IF(blockIndex >= pt->blockCount, DFS_NVAL_ARGS);

	uint32_t offset = blockIndex & 0x7; 
	uint32_t index = blockIndex & ~0x7;

	*used = pt->blockMap->map[index] & (1 << offset);

	return DFS_SUCCESS;
}

int SetBlockUsed(const DPartition *pt, uint32_t blockIndex, bool used)
{
	ERR_NULL(pt, DFS_NVAL_ARGS);
	ERR_IF(blockIndex >= pt->blockCount, DFS_NVAL_ARGS);

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
	ERR_NULL(pt, DFS_NVAL_ARGS);

	DeviceSeek(SEEK_SET, sizeof(PartitionHeader), pt);
	size_t written = DeviceWrite(pt->blockMap->map, pt->blockMap->length, pt);

	ERR_IF(written != pt->blockMap->length, DFS_FAILED_DEVICE_WRITE);

	return DFS_SUCCESS;
}

int DestroyBlockMap(DPartition *pt)
{
	ERR_NULL(pt, DFS_NVAL_ARGS);

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
	return fwrite(buffer, len, 1, partition->device);
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
inline size_t DeviceWriteAtEntryLoc(const EntryPointerLoc entryLoc, void *buffer, const DPartition *partition)
{
	size_t addr = EntryLocToAddr(partition, entryLoc);
	DeviceSeek(SEEK_SET, addr, partition);
	return DeviceWrite(buffer, sizeof(EntryPointer), partition);
}

inline size_t DeviceRead(void *buffer, const size_t len, const DPartition *partition)
{
	return fread(buffer, len, 1, partition->device);
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
	ERR_NULL(device, DFS_NVAL_ARGS);
	ERR_NULL(size, DFS_NVAL_ARGS);
	ERR_IF(size & 0xFFF, DFS_NVAL_ARGS); //size % 4096

	FILE* file = fopen(device, "w+b");
	char zero = 0;

	ERR_NULL(file, DFS_FAILED_DEVICE_OPEN);

	/*char buff[4096];
	memset(buff, 0, sizeof(buff));
	size_t times = size >> 12; //size / 4096;

	while (times)
	{
		size_t written = fwrite(buff, 1, 4096, file);

		ERR_IF_CLEANUP(written != 4096,
			DFS_FAILED_SPACE_RESERVE, fclose(file));
		times--;
	}*/

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
	size_t baseAddress = BlkIdxToAddr(partition, entryLoc.blockIndex);
	size_t offset = sizeof(BlockHeader) + sizeof(EntryPointer) * entryLoc.entryIndex;

	return baseAddress + offset;
}
#pragma endregion
#pragma region Code naming
size_t DetermineFirstBlockAddress(uint32_t blockMapSize)
{
	size_t withoutPad = (size_t)blockMapSize + sizeof(PartitionHeader);
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

int ValidatePartitionHeader(const DPartition* pt)
{
	if (!pt)
		return DFS_NVAL_ARGS;

	PartitionHeader buff;
	size_t read;
	
	read = fread(&buff, sizeof(PartitionHeader), 1, pt->device);
	ERR_IF(read != sizeof(PartitionHeader), DFS_FAILED_DEVICE_READ);

	//Check magic number
	ERR_IF(buff.magicNumber != MAGIC_NUMBER, DFS_CORRUPTED_FS);

	//Check reserved
	ERR_IF(buff.resvd != 0, DFS_CORRUPTED_FS);

	//Check blockCount will not overflow
	ERR_IF(buff.blockMapSize > (~0u >> 3), DFS_CORRUPTED_FS);

	return DFS_SUCCESS;
}
#pragma endregion
#pragma region Block navigation
int FindEntryPointer(const DPartition* pt, const char *path, EntryPointer *entry, EntryPointerLoc *entryLoc)
{
	//This is the 'intermediate' method
	//Validates arguments and initiates recursion

	ERR_NULL(pt, DFS_NVAL_ARGS);
	ERR_NULL(path, DFS_NVAL_ARGS);

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
	size_t nextIdx = 0;

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
	ERR_NULL(pt, DFS_NVAL_ARGS);
	ERR_NULL(index, DFS_NVAL_ARGS);

	int err;
	uint32_t i;

	//TODO: Optimize to use blockMap byte searches (8 bits at a time)
	for (i = 0; i < pt->blockCount; i++)
	{
		bool used;
		ERR_NZERO((err = GetBlockUsed(pt, i, &used)), err);

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
	ERR_NULL(pt, DFS_NVAL_ARGS);

	int err; size_t read;
	uint32_t newBlockIndex, oldBlockIndex;
	EntryPointer entry;
	BlockHeader newBlock, oldBlock;

	//Find block and reserve
	ERR_NZERO((err = FindFreeBlock(pt, &newBlockIndex)), err);
	ERR_NZERO((err = SetBlockUsed(pt, newBlockIndex, true)), err);
	ERR_NZERO((err = FlushFullBlockMap(pt)), err);

	//Read entry pointer
	read = DeviceReadAtEntryLoc(entryLoc, &entry, pt);
	ERR_IF(read != sizeof(EntryPointer), DFS_FAILED_DEVICE_READ); //TODO: Revert block set to used

	//Update last block index in entry
	oldBlockIndex = entry.lastBlock;
	entry.lastBlock = newBlockIndex;

	//Flush changes
	read = DeviceWriteAtEntryLoc(entryLoc, &entry, pt);
	ERR_IF(read != sizeof(EntryPointer), DFS_FAILED_DEVICE_WRITE); //TODO: Revert block set to used

	//Create new block header
	newBlock.nextBlock = 0;
	newBlock.prevBlock = oldBlockIndex;
	newBlock.usedSpace = 0;
	newBlock.resvd = 0;

	//Store new block
	read = DeviceWriteAtBlk(newBlockIndex, &newBlock, sizeof(BlockHeader), pt);
	ERR_IF(read != sizeof(BlockHeader), DFS_FAILED_DEVICE_WRITE);

	//Read old block
	read = DeviceReadAtBlk(oldBlockIndex, &oldBlock, sizeof(BlockHeader), pt);
	ERR_IF(read != sizeof(BlockHeader), DFS_FAILED_DEVICE_READ);

	//Update index
	oldBlock.nextBlock = newBlockIndex;

	//Flush changes
	read = DeviceWriteAtBlk(oldBlockIndex, &oldBlock, sizeof(BlockHeader), pt);
	ERR_IF(read != sizeof(BlockHeader), DFS_FAILED_DEVICE_WRITE);

	if (newBlkIdx)
		*newBlkIdx = newBlockIndex;

	return DFS_SUCCESS;
}

int AppendEntryToDir(const DPartition *pt, const EntryPointerLoc dirEntryLoc, EntryPointer newEntry)
{
	ERR_NULL(pt, DFS_NVAL_ARGS);

	EntryPointer dirEntry;
	BlockHeader dirBlock;
	uint32_t blockIndex;
	int err; size_t read;

	//Read dir entry
	read = DeviceReadAtEntryLoc(dirEntryLoc, &dirEntry, pt);
	ERR_IF(read != sizeof(EntryPointer), DFS_FAILED_DEVICE_READ);

	//Load last block
	blockIndex = dirEntry.lastBlock;
	read = DeviceReadAtBlk(blockIndex, &dirBlock, sizeof(BlockHeader), pt);
	ERR_IF(read != sizeof(BlockHeader), DFS_FAILED_DEVICE_READ);

	if (dirBlock.usedSpace + sizeof(EntryPointer) >= BLOCK_DATA_SIZE) //No free space
	{
		//Append block and update block index
		ERR_NZERO((err = AppendBlockToFile(pt, dirEntryLoc, &blockIndex)), err);

		//Load new block
		read = DeviceReadAtBlk(blockIndex, &dirBlock, sizeof(BlockHeader), pt);
		ERR_IF(read != sizeof(BlockHeader), DFS_FAILED_DEVICE_READ); //TODO: Revert changes
	}

	//Write new entry pointer
	size_t addr = BlkIdxToAddr(pt, blockIndex) + dirBlock.usedSpace;
	read = DeviceReadAt(addr, &newEntry, sizeof(EntryPointer), pt);
	ERR_IF(read != sizeof(EntryPointer), DFS_FAILED_DEVICE_WRITE); //TODO: Revert changes

	//Update block header
	dirBlock.usedSpace += sizeof(EntryPointer);

	//Flush header changes
	read = DeviceReadAtBlk(blockIndex, &dirBlock, sizeof(BlockHeader), pt);
	ERR_IF(read != sizeof(BlockHeader), DFS_FAILED_DEVICE_WRITE); //TODO: Revert changes

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

	DPathGetParent(parentDir, path);
	DPathGetName(name, path);

	//Find parent dir
	ERR_NZERO((err = FindEntryPointer(pt, parentDir, NULL, &parentLoc)), err);

	//Find and reserve free block
	ERR_NZERO((err = FindFreeBlock(pt, &newBlockIndex)), err);
	ERR_NZERO((err = SetBlockUsed(pt, newBlockIndex, true)), err);
	ERR_NZERO((err = FlushFullBlockMap(pt)), err);

	//Create new entry
	strncpy(newEntry.name, name, MAX_PATH_NAME);
	newEntry.firstBlock = newBlockIndex;
	newEntry.lastBlock = newBlockIndex;
	newEntry.resvd = 0;
	newEntry.flags = flags;

	//Append entry to parent
	ERR_NZERO((err = AppendEntryToDir(pt, parentLoc, newEntry)), err);

	//Set new block header
	newBlock.nextBlock = 0;
	newBlock.prevBlock = 0;
	newBlock.usedSpace = 0;
	newBlock.resvd = 0;

	//Flush changes
	ERR_NZERO((err = DeviceReadAtBlk(newBlockIndex, &newBlock, sizeof(BlockHeader), pt)), err);

	return DFS_SUCCESS;
}

int DetermineFileSize(DPartition *pt, const EntryPointer entry, size_t *size)
{
	ERR_NULL(pt, DFS_NVAL_ARGS);
	ERR_NULL(size, DFS_NVAL_ARGS);

	size_t counter = 0;
	BlockHeader curBlock;
	uint32_t blockIndex = entry.firstBlock;
	int err;

	while (blockIndex) //First should always go through since there should be no entries with null/root block
	{
		err = DeviceReadAtBlk(blockIndex, &curBlock, sizeof(BlockHeader), pt);
		ERR_NZERO(err, err);

		counter += curBlock.usedSpace; //In theory all but last should be full
		blockIndex = curBlock.nextBlock;
	}
	
	*size = counter;

	return DFS_SUCCESS;
}
#pragma endregion
