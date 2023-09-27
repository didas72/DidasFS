//DidasFS_structures.h - Defines internal structures

#ifndef DIDASFS_STRUCTURES_H
#define DIDASFS_STRUCTURES_H

#include <stdint.h>
#include <stdbool.h>
#include "DidasFS.h"

#define SECTOR_SIZE 512
#define BLOCK_SIZE 32768
#define BLOCK_DATA_SIZE 32736
#define ENTRIES_PER_BLOCK 1023
#define MAGIC_NUMBER 0x69ADDE69

#pragma region Entry flags
#define ENTRY_FLAG_EMPTY 0
#define ENTRY_FLAG_FILE 0x0
#define ENTRY_FLAG_DIR 0x1
#define ENTRY_FLAG_READWRITE 0x0
#define ENTRY_FLAG_READONLY 0x2
#pragma endregion

#pragma region File stream flags
#define FILESTREAM_FLAG_EMPTY 0
#define FILESTREAM_FLAG_READ 0x1
#define FILESTREAM_FLAG_WRITE 0x2
#pragma endregion



//==="Private" logical representations===
typedef struct 
{
	size_t length;
	uint8_t *map;
	DPartition *host;
} BlockMap;

//Set to -1, -1 for root
typedef struct
{
	uint32_t blockIndex;
	uint32_t entryIndex;
} EntryPointerLoc;


//==="Public" logical representations===
typedef struct DPartition
{
	FILE *device;
	size_t rootBlockAddr;
	uint32_t blockCount;
	BlockMap *blockMap;
} _DPartition;

typedef struct DFileStream
{
	//Position in file
	size_t filePos;
	size_t fileSize;
	uint32_t curBlockIdx, firstBlockIdx, lastBlockIdx;
	uint16_t streamFlags;
	EntryPointerLoc entryLoc;
	DPartition *pt;
} _DFileStream;


//===Physical representations===
typedef struct
{
	uint32_t magicNumber;
	uint32_t blockMapSize;
	uint64_t resvd;
} __attribute__((packed)) PartitionHeader;

typedef struct
{
	uint32_t prevBlock;
	uint32_t nextBlock;
	uint32_t usedSpace;
	uint32_t resvd;
} __attribute__((packed)) BlockHeader;

typedef struct 
{
	uint32_t firstBlock;
	uint32_t lastBlock;
	uint16_t flags;
	uint16_t resvd;
	char name[20];
} __attribute__((packed)) EntryPointer;


//"Private" logical representation methods
int LoadBlockMap(DPartition *host);
int GetBlockUsed(const DPartition *pt, uint32_t blockIndex, bool *used);
int SetBlockUsed(const DPartition *pt, uint32_t blockIndex, bool used);
int FlushFullBlockMap(const DPartition *pt);
//int FlushBlockMapChanges(DPartition *pt);
int DestroyBlockMap(DPartition *pt);
#endif
