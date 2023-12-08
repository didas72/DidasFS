//didasFS.h - Defines public interface of the file system

#ifndef DIDASFS_H
#define DIDASFS_H

#include <stddef.h>
#include <stdint.h>



///@brief Holds an error code
typedef int DFS_err;
///@brief Holds file mode flags
typedef uint32_t DFS_filem_flags;
///@brief Represents a partition handle
typedef struct DPartition DPartition;
///@brief Represents a file handle
typedef struct DFileHandle DFileHandle;


//===Error codes===
///@brief Function has not been (fully) implemented
#define DFS_NOT_IMPLEMENTED (DFS_err)-1
///@brief Function has performed the operation successfully
#define DFS_SUCCESS (DFS_err)0
///@brief Function failed for an unspecified reason
#define DFS_FAIL (DFS_err)1
///@brief Function was given invalid arguments
#define DFS_NVAL_ARGS (DFS_err)2
///@brief Function was unable to open the requested file/device
#define DFS_FAILED_DEVICE_OPEN (DFS_err)3
///@brief [UNUSED] Function failed to reserve the required file/device space
#define DFS_FAILED_SPACE_RESERVE (DFS_err)4
///@brief Function failed to write to the underlying file/device
#define DFS_FAILED_DEVICE_WRITE (DFS_err)5
///@brief Function failed to read from the underlying file/device
#define DFS_FAILED_DEVICE_READ (DFS_err)6
///@brief Function failed to allocate the required memory
#define DFS_FAILED_ALLOC (DFS_err)7
///@brief Function determined that the used partition is corrupted
#define DFS_CORRUPTED_FS (DFS_err)8
///@brief Function encountered invalid flags
#define DFS_NVAL_FLAGS (DFS_err)9
///@brief Function failed to find part of the requested path
#define DFS_PATH_NOT_FOUND (DFS_err)10
///@brief Function failed to allocate space in the partition
#define DFS_NO_SPACE (DFS_err)11
///@brief Function received an invalid seek position
#define DFS_NVAL_SEEK (DFS_err)12

//===File mode flags===
#define DFS_READ (DFS_filem_flags)0x0001
#define DFS_WRITE (DFS_filem_flags)0x0002
#define DFS_SHARE_READ (DFS_filem_flags)0x0004
///@brief [NOT IMPLEMENTED]
#define DFS_NO_BUFFERING (DFS_filem_flags)0x0800



/**
 * @brief Initializes a partition on the given file/device
 * 
 * @param device Path to the file/device to use
 * @param dataSize The size of the data space to allocate
 * @return int containing the error code for the operation
 */
DFS_err dpcreate(const char *device, size_t totalSize);
/**
 * @brief Opens an existing partition from a file/device
 * 
 * @param device Path to the file/device to use
 * @param ptHandle Pointer to a partition handle pointer
 * @return int containing the error code for the operation
 */
DFS_err dpopen(const char *device, DPartition **ptHandle);
/**
 * @brief Closes an open partition, releasing all associated resources
 * 
 * @param ptHandle Pointer to a partition handle
 * @return int containing the error code for the operation
 */
DFS_err dpclose(DPartition *ptHandle);

/**
 * @brief Creates an empty directory at the specified path
 * 
 * @param pt Pointer to a partition handle to be used
 * @param path Path of the directory to be created
 * @return int containing the error code for the operation
 */
DFS_err ddcreate(DPartition *pt, const char *path);
/**
 * @brief Create an empty file at the specified path
 * 
 * @param pt Pointer to a partition handle to be used
 * @param path Path of the directory to be created
 * @return int containing the error code for the operation
 */
DFS_err dfcreate(DPartition *pt, const char *path);

/**
 * @brief Opens an existing file at the specified path
 * 
 * @param pt Pointer to a partition handle to be used
 * @param path Path of the directory to be created
 * @param fsHandle Pointer to the file handle pointer to populate
 * @return int containing the error code for the operation
 */
DFS_err dfopen(DPartition *pt, const char *path, const DFS_filem_flags flags, int *handle);
/**
 * @brief Closes an open file handle and flushes and buffered changes
 * 
 * @param fs Pointer to the file handle to be closed
 * @return int containing the error code for the operation
 */
DFS_err dfclose(DPartition *pt, int handle);

/**
 * @brief Writes a block of data to a stream
 * 
 * @param buffer Pointer to the buffer to write the data from
 * @param len Length in bytes of the data to be written
 * @param fs Pointer to the file handle to write to
 * @param written Referenced variable will be set to the actual number of bytes written
 * @return int containing the error code for the operation
 */
DFS_err dfwrite(DPartition *pt, int handle, void *buffer, size_t len, size_t *written);
/**
 * @brief Reads a block of data from a stream
 * 
 * @param buffer Pointer to a buffer to read the data to
 * @param len Length in bytes of the data to be read
 * @param fs Pointer to the file handle to read from
 * @param written Referenced variable will be set to the actual number of bytes read
 * @return int containing the error code for the operation
 */
DFS_err dfread(DPartition *pt, int handle, void *buffer, size_t len, size_t *read);
/**
 * @brief Sets the stream position
 * 
 * @param pos The new position to set the stream to
 * @param fs Pointer to the file handle to modify
 * @return int containing the error code for the operation
 */
DFS_err dfseek(DPartition *pt, int handle, size_t pos /*TODO: WHENCE and return position*/);
#endif