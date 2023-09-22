//DidasFS_structures.h - Defines internal structures

#ifndef DIDASFS_STRUCTURES_H
#define DIDASFS_STRUCTURES_H

#include <stdint.h>
#include <stdbool.h>

#define SECTOR_SIZE 512
#define BLOCK_SIZE 32768
#define BLOCK_DATA_SIZE 32736
#define ENTRIES_PER_BLOCK 1023
#define MAGIC_NUMBER 0x69ADDE69

#define ENTRY_FLAG_DIR 1


typedef struct BlockMap BlockMap;


//"Public" logical representations
typedef struct DPartition
{
	FILE *device;
	size_t rootBlockAddr;
	uint32_t blockCount;
	BlockMap *blockMap;
} _DPartition;

typedef struct DFileStream
{
	size_t filePos;
	uint32_t curBlockIdx, firstBlockIdx, lastBlockIdx;
	uint16_t streamFlags, fileFlags;
} _DFileStream;


//"Private" logical representations
typedef struct BlockMap
{
	size_t length;
	uint8_t *map;
	DPartition *host;
} _BlockMap;


//Physical representations
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
	uint32_t resvd1;
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
int GetBlockUsed(DPartition *pt, uint32_t blockIndex, bool *used);
int SetBlockUsed(DPartition *pt, uint32_t blockIndex, bool used);
int FlushFullBlockMap(DPartition *pt);
//int FlushBlockMapChanges(DPartition *pt);
int DetroyBlockMap(DPartition *pt);
#endif
