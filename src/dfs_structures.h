//didasFS_structures.h - Defines internal structures

#ifndef DIDASFS_STRUCTURES_H
#define DIDASFS_STRUCTURES_H

#include <stdint.h>
#include <stdbool.h>

#include "dfs.h"
#include "structures/hashmap.h"
#include "paths.h"

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



typedef uint32_t blk_idx_t;
typedef uint16_t file_flags_t;



//==="Private" logical representations===
typedef struct
{
	size_t length;
	uint8_t *map;
	dfs_partition *host;
} BlockMap;

//Set to -1, -1 for root
typedef struct
{
	blk_idx_t blockIndex;
	uint32_t entryIndex;
} EntryPointerLoc;


//==="Public" logical representations===
typedef struct dfs_partition
{
	int device;
	size_t rootBlockAddr;
	uint32_t blockCount;
	BlockMap *blockMap;
	Hashmap *fileHandles;
} _dfs_partition;

typedef struct dfs_file
{
	//Handle tracking
	char path[MAX_PATH];
	dfs_filem_flags flags;

	//Positioning
	size_t head;
	size_t file_size;
	blk_idx_t cur_blk_idx, first_blk_idx, last_blk_idx;
	EntryPointerLoc entry_loc;

	//Partition not needed, will be passed to every call
} _dfs_file;


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
dfs_err load_blk_map(dfs_partition *host);
dfs_err get_blk_used(const dfs_partition *pt, blk_idx_t blockIndex, bool *used);
dfs_err set_blk_used(const dfs_partition *pt, blk_idx_t blockIndex, bool used);
dfs_err flush_full_blk_map(const dfs_partition *pt);
//int FlushBlockMapChanges(dfs_partition *pt);
dfs_err destroy_blk_map(dfs_partition *pt);
#endif
