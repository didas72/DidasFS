//didasFS_internals.h - 

#ifndef DIDASFS_INTERNALS_H
#define DIDASFS_INTERNALS_H

#include <stdint.h>
#include <stddef.h>

#include "didasFS.h"
#include "didasFS_structures.h"


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
size_t DetermineSizeFromBlockCount(size_t blockCount);
size_t DetermineBlockCount(size_t maxSize, size_t *partitionSize);
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

#endif
