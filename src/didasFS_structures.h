//didasFS_structures.h - Defines internal structures

#ifndef DIDASFS_STRUCTURES_H
#define DIDASFS_STRUCTURES_H

#include <stdint.h>
#include <stdbool.h>

#include "didasFS.h"
#include "structures/hashmap.h"
#include "dPaths.h"

#define SECTOR_SIZE 512
#define BLOCK_SIZE 32768
#define BLOCK_DATA_SIZE 32752
#define ENTRIES_PER_BLK 1023
#define MAGIC_NUMBER 0x69ADDE69
#define MAX_BLKS 0xFFFFFFFF
#define MAX_PARTITION_CAPACITY MAX_BLKS * BLOCK_DATA_SIZE

#pragma region Entry flags
#define ENTRY_FLAG_EMPTY 0x00
#define ENTRY_FLAG_FILE 0x00
#define ENTRY_FLAG_DIR 0x01
#define ENTRY_FLAG_READWRITE 0x00
#define ENTRY_FLAG_READONLY 0x02
#pragma endregion



typedef uint32_t DFS_filem_flags;
typedef uint32_t blk_idx_t;
typedef uint16_t file_flags_t;



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
	blk_idx_t blockIndex;
	uint32_t entryIndex;
} EntryPointerLoc;


//==="Public" logical representations===
typedef struct DPartition
{
	int device;
	size_t rootBlockAddr;
	uint32_t blockCount;
	BlockMap *blockMap;
	Hashmap *fileHandles;
} _DPartition;

typedef struct DFileHandle
{
	//Handle tracking
	char path[MAX_PATH];
	DFS_filem_flags flags;

	//Positioning
	size_t head;
	size_t file_size;
	blk_idx_t cur_blk_idx, first_blk_idx, last_blk_idx;
	EntryPointerLoc entry_loc;

	//Partition not needed, will be passed to every call
} _DFileHandle;

//TODO: Decomission
typedef struct DFileStream
{
	//Position in file
	size_t filePos;
	size_t fileSize;
	blk_idx_t curBlockIdx, firstBlockIdx, lastBlockIdx;
	uint16_t streamFlags;
	EntryPointerLoc entryLoc;
	DPartition *pt;
} _DFileStream;


//==="Physical" representations===
typedef struct
{
	uint32_t magicNumber;
	uint32_t blockMapSize;
	uint64_t resvd;
} __attribute__((packed)) PartitionHeader;

typedef struct
{
	blk_idx_t prevBlock;
	blk_idx_t nextBlock;
	uint32_t usedSpace;
	uint32_t resvd;
} __attribute__((packed)) BlockHeader;

typedef struct 
{
	blk_idx_t firstBlock;
	blk_idx_t lastBlock;
	file_flags_t flags;
	uint16_t resvd;
	char name[20];
} __attribute__((packed)) EntryPointer;


//"Private" logical representation methods
DFS_err load_blk_map(DPartition *host);
DFS_err get_blk_used(const DPartition *pt, blk_idx_t blockIndex, bool *used);
DFS_err set_blk_used(const DPartition *pt, blk_idx_t blockIndex, bool used);
DFS_err flush_full_blk_map(const DPartition *pt);
//int FlushBlockMapChanges(DPartition *pt);
DFS_err destroy_blk_map(DPartition *pt);
#endif
