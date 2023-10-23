//didasFS.h - Defines public interface of the file system

#ifndef DIDASFS_H
#define DIDASFS_H

//===Error codes===
/**
 * @brief Function has not been (fully) implemented
 * 
 */
#define DFS_NOT_IMPLEMENTED -1
/**
 * @brief Function has performed the operation successfully
 * 
 */
#define DFS_SUCCESS 0
/**
 * @brief Function failed for an unspecified reason
 * 
*/
#define DFS_FAIL 1
/**
 * @brief Function was given invalid arguments
 * 
*/
#define DFS_NVAL_ARGS 2
/**
 * @brief Function was unable to open the requested file/device
 * 
*/
#define DFS_FAILED_DEVICE_OPEN 3
/**
 * @brief Function failed to reserve the required file/device space
 * 
*/
#define DFS_FAILED_SPACE_RESERVE 4
/**
 * @brief Function failed to write to the underlying file/device
 * 
*/
#define DFS_FAILED_DEVICE_WRITE 5
/**
 * @brief Function failed to read from the underlying file/device
 * 
*/
#define DFS_FAILED_DEVICE_READ 6
/**
 * @brief Function failed to allocate the required memory
 * 
*/
#define DFS_FAILED_ALLOC 7
/**
 * @brief Function determined that the used partition is corrupted
 * 
*/
#define DFS_CORRUPTED_FS 8
/**
 * @brief Function encountered invalid flags
 * 
*/
#define DFS_NVAL_FLAGS 9
/**
 * @brief Function failed to find part of the requested path
 * 
*/
#define DFS_PATH_NOT_FOUND 10
/**
 * @brief Function failed to allocate space in the partition
 * 
*/
#define DFS_NO_SPACE 11
/**
 * @brief Function received an invalid seek position
 * 
*/
#define DFS_NVAL_SEEK 12



/**
 * @brief Represents a partition handle
 * 
 */
typedef struct DPartition DPartition;
/**
 * @brief Represents a file handle
 * 
 */
typedef struct DFileStream DFileStream;

/**
 * @brief Initializes a partition on the given file/device
 * 
 * @param device Path to the file/device to use
 * @param dataSize The size of the data space to allocate
 * @return int containing the error code for the operation
 */
int InitPartition(const char *device, size_t dataSize);
/**
 * @brief Opens an existing partition from a file/device
 * 
 * @param device Path to the file/device to use
 * @param ptHandle Pointer to a partition handle pointer
 * @return int containing the error code for the operation
 */
int OpenPartition(const char *device, DPartition **ptHandle);
/**
 * @brief Closes an open partition, releasing all associated resources
 * 
 * @param ptHandle Pointer to a partition handle
 * @return int containing the error code for the operation
 */
int ClosePartition(DPartition *ptHandle);

/**
 * @brief Creates an empty directory at the specified path
 * 
 * @param pt Pointer to a partition handle to be used
 * @param path Path of the directory to be created
 * @return int containing the error code for the operation
 */
int CreateDirectory(DPartition *pt, const char *path);
/**
 * @brief Create an empty file at the specified path
 * 
 * @param pt Pointer to a partition handle to be used
 * @param path Path of the directory to be created
 * @return int containing the error code for the operation
 */
int CreateFile(DPartition *pt, const char *path);

/**
 * @brief Opens an existing file at the specified path
 * 
 * @param pt Pointer to a partition handle to be used
 * @param path Path of the directory to be created
 * @param fsHandle Pointer to the file handle pointer to populate
 * @return int containing the error code for the operation
 */
int OpenFile(DPartition *pt, const char *path, DFileStream **fsHandle);
/**
 * @brief Closes an open file handle and flushes and buffered changes
 * 
 * @param fs Pointer to the file handle to be closed
 * @return int containing the error code for the operation
 */
int CloseFile(DFileStream *fs);

/**
 * @brief Writes a block of data to a stream
 * 
 * @param buffer Pointer to the buffer to write the data from
 * @param len Length in bytes of the data to be written
 * @param fs Pointer to the file handle to write to
 * @param written Referenced variable will be set to the actual number of bytes written
 * @return int containing the error code for the operation
 */
int FileWrite(void *buffer, size_t len, DFileStream *fs, size_t *written);
/**
 * @brief Reads a block of data from a stream
 * 
 * @param buffer Pointer to a buffer to read the data to
 * @param len Length in bytes of the data to be read
 * @param fs Pointer to the file handle to read from
 * @param written Referenced variable will be set to the actual number of bytes read
 * @return int containing the error code for the operation
 */
int FileRead(void *buffer, size_t len, DFileStream *fs, size_t *read);
/**
 * @brief Sets the stream position
 * 
 * @param pos The new position to set the stream to
 * @param fs Pointer to the file handle to modify
 * @return int containing the error code for the operation
 */
int FileSetPos(size_t pos, DFileStream *fs);
#endif
