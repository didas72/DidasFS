//DidasFS.h - Defines public interface of the file system

#ifndef DIDASFS_H
#define DIDASFS_H

#include <stdio.h>

//Error codes
#define DFS_NOT_IMPLEMENTED -1
#define DFS_SUCCESS 0
#define DFS_FAIL 1
#define DFS_NVAL_ARGS 2
#define DFS_FAILED_DEVICE_OPEN 3
#define DFS_FAILED_SPACE_RESERVE 4
#define DFS_FAILED_DEVICE_WRITE 5
#define DFS_FAILED_DEVICE_READ 6
#define DFS_FAILED_ALLOC 7
#define DFS_CORRUPTED_FS 8
#define DFS_NVAL_FLAGS 9
#define DFS_PATH_NOT_FOUND 10
#define DFS_NO_SPACE 11

typedef struct DPartition DPartition;
typedef struct DFileStream DFileStream;

int InitFileSystem(const char *device, size_t dataSize);
int OpenFileSystem(const char *device, DPartition **ptHandle);
int CloseFileSystem(DPartition *ptHandle);

int CreateDirectory(DPartition *pt, const char *path);
int CreateFile(DPartition *pt, const char *path);
int OpenFile(DPartition *pt, const char *path, DFileStream **fsHandle);
int CloseFile(DPartition *pt, DFileStream *fsHandle);
#endif

//TODO: Replace all fread to work with 512 multiples for hardware devicess

//TODO: Update OpenFile to register file handle
//TODO: Update OpenFile to check for already open handles
//TODO: Update OpenFile to check file flags and match handle type
//TODO: Update CloseFile to remove file handle
//TODO: Change Partition creation to use total size instead of (incorrect) data size
