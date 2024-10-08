//dfs_structures.h - Defines internal structures

#ifndef DIDASFS_STRUCTURES_H
#define DIDASFS_STRUCTURES_H

#include <stdint.h>
#include <stdbool.h>

#include "dfs.h"
#include "paths.h"

#define SECTOR_SIZE 512
#define BLOCK_SIZE 32768
#define BLOCK_DATA_SIZE 32752
#define ENTRIES_PER_BLK 1023
#define MAGIC_NUMBER 0x69ADDE69
#define MAX_BLKS 0xFFFFFFFF
#define MAX_PARTITION_CAPACITY MAX_BLKS * BLOCK_DATA_SIZE

#pragma region Entry flags
#define ENTRY_FLAG_EMPTY (file_flags_t)0x0000
#define ENTRY_FLAG_FILE (file_flags_t)0x0000
#define ENTRY_FLAG_DIR (file_flags_t)0x0001
#define ENTRY_FLAG_READWRITE (file_flags_t)0x0000
#define ENTRY_FLAG_READONLY (file_flags_t)0x0002
#define ENTRY_FLAG_SYSTEM (file_flags_t)0x0004
#define ENTRY_FLAG_HIDDEN (file_flags_t)0x0008
#pragma endregion



//===Types===
typedef uint32_t blk_idx_t;
typedef uint16_t file_flags_t;



//==="Private" logical representations===
typedef struct
{
	size_t length;
	uint8_t *map;
} blk_map;

//Set to -1, -1 for root
typedef struct
{
	blk_idx_t blk_idx;
	uint32_t entry_idx;
} entry_ptr_loc;

typedef struct
{
	//Handle tracking
	bool present;
	char path[MAX_PATH];
	dfs_filem_flags flags;

	//Positioning
	size_t head;
	blk_idx_t cur_blk_idx, first_blk_idx, last_blk_idx;
	entry_ptr_loc entry_loc;
} dfs_file;

struct dfs_partition
{
	int device;
	size_t root_blk_addr;
	uint32_t blk_count;
	blk_map *usage_map;
	dfs_file open_handles[DFS_MAX_HANDLES];
};



//==="Physical" representations===
typedef struct
{
	uint32_t magic_number;
	blk_idx_t block_count;
	uint64_t resvd;
} __attribute__((packed)) partition_header;

typedef struct
{
	blk_idx_t prev_blk;
	blk_idx_t next_blk;
	uint32_t used_space; //Could be 16-bit since block can hold up to 32K-16 < 64K
	uint32_t resvd;
} __attribute__((packed)) block_header;

typedef struct 
{
	blk_idx_t first_blk;
	blk_idx_t last_blk;
	file_flags_t flags;
	uint16_t resvd;
	char name[MAX_PATH_NAME];
} __attribute__((packed)) entry_pointer;


//"Private" logical representation methods
static int get_lowest_unused_descriptor(const dfs_partition pt);

static dfs_err load_blk_map(dfs_partition *host);
static dfs_err get_blk_used(const dfs_partition *pt, blk_idx_t blk_idx, bool *used);
static dfs_err set_blk_used(const dfs_partition *pt, blk_idx_t blk_idx, bool used);
static dfs_err flush_full_blk_map(const dfs_partition *pt);
//static int flush_blk_map_changes(dfs_partition *pt);
static dfs_err destroy_blk_map(dfs_partition *pt);
#endif
