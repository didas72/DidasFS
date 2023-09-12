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

typedef struct DPartition DPartition;
typedef struct DFileStream DFileStream;

int InitFileSystem(char *device, size_t dataSize);
int OpenFileSystem(char *device, DPartition **ptHandle);

int OpenFile(DPartition *pt, char *path, DFileStream **fsHandle);
#endif

//TODO: OpenFile

