//DidasFS_structures.h - Defines internal structures

#ifndef DIDASFS_STRUCTURES_H
#define DIDASFS_STRUCTURES_H

#include <stdint.h>

#define BLOCK_SIZE 32768
#define BLOCK_DATA_SIZE 32736
#define MAGIC_NUMBER 0x69DEAD69



typedef struct DPartition
{
	FILE *device;
	size_t rootBlockAddr;
} _DPartition;

typedef struct DFileStream
{
	size_t filePos;
	size_t curBlockAddr, firstBlockAddr, lastBlockAddr;
} _DFileStream;

typedef struct
{
	uint32_t magicNumber;
	uint32_t blockMapSize;
	uint64_t resvd;
} __attribute__((packed)) PartitionHeader;

typedef struct
{
	uint64_t prevBlock;
	uint64_t nextBlock;
	uint32_t usedSpace;
	uint32_t resvd1;
	uint64_t resvd2;
} __attribute__((packed)) BlockHeader;

typedef struct 
{
	uint64_t firstBlock;
	uint64_t lastBlock;
	uint32_t resvd;
	char name[28];
} __attribute__((packed)) EntryPointer;


#endif
