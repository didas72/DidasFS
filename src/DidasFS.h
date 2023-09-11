//DidasFS.h - Defines public interface of the project

#ifndef DIDASFS_H
#define DIDASFS_H

#include <stdio.h>

//Error codes
#define DFS_SUCCESS 0
#define DFS_FAIL 1
#define DFS_NVAL_ARGS 2
#define DFS_FAILED_DEVICE_OPEN 3
#define DFS_FAILED_SPACE_RESERVE 4
#define DFS_FAILED_DEVICE_WRITE 5
#define DFS_FAILED_DEVICE_READ 6

typedef struct DidasFS DidasFS;

int InitFileSystem(char *device, size_t dataSize);
//int OpenFileSystem(char *device);

/*int OpenFile(char *path, DFile *file);
int CloseFile(DFile *file);*/

#endif
