//DidasFS_structures.h - Defines internal structures

#ifndef DIDASFS_STRUCTURES_H
#define DIDASFS_STRUCTURES_H

#include <stdint.h>

#define BLOCK_SIZE 32768
#define BLOCK_DATA_SIZE 32736
#define ENTRIES_PER_BLOCK 1023
#define MAGIC_NUMBER 0x69DEAD69

#define ENTRY_FLAG_DIR 1



typedef struct DPartition
{
	FILE *device;
	size_t rootBlockAddr;
} _DPartition;

typedef struct DFileStream
{
	size_t filePos;
	uint32_t curBlockIdx, firstBlockIdx, lastBlockIdx;
	uint16_t streamFlags, fileFlags;
} _DFileStream;

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


#endif
