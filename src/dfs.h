//didasFS.h - Defines public interface of the file system

#ifndef DIDASFS_H
#define DIDASFS_H

#include <stddef.h>
#include <stdint.h>


//===Types===
///@brief Holds an error code
typedef int dfs_err;
///@brief Holds file mode flags
typedef uint32_t dfs_filem_flags;
///@brief Represents a partition handle
typedef struct dfs_partition dfs_partition;
///@brief Represents a file handle
typedef struct dfs_file dfs_file;


//===Constants===
#define DFS_MAX_HANDLES 64


//===Error codes===
///@brief Function has not been (fully) implemented
#define DFS_NOT_IMPLEMENTED (dfs_err)-1
///@brief Function has performed the operation successfully
#define DFS_SUCCESS (dfs_err)0
///@brief Function failed for an unspecified reason
#define DFS_FAIL (dfs_err)1
///@brief Function was given invalid arguments
#define DFS_NVAL_ARGS (dfs_err)2
///@brief Function was unable to open the requested file/device
#define DFS_FAILED_DEVICE_OPEN (dfs_err)3
///@brief [UNUSED] Function failed to reserve the required file/device space
#define DFS_FAILED_SPACE_RESERVE (dfs_err)4
///@brief Function failed to write to the underlying file/device
#define DFS_FAILED_DEVICE_WRITE (dfs_err)5
///@brief Function failed to read from the underlying file/device
#define DFS_FAILED_DEVICE_READ (dfs_err)6
///@brief Function failed to allocate the required memory
#define DFS_FAILED_ALLOC (dfs_err)7
///@brief Function determined that the used partition is corrupted
#define DFS_CORRUPTED_PARTITION (dfs_err)8
///@brief Function encountered invalid flags
#define DFS_NVAL_FLAGS (dfs_err)9
///@brief Function failed to find part of the requested path
#define DFS_PATH_NOT_FOUND (dfs_err)10
///@brief Function failed to allocate space in the partition
#define DFS_NO_SPACE (dfs_err)11
///@brief Function received an invalid seek position
#define DFS_NVAL_SEEK (dfs_err)12
///@brief Failed to open file because maximum number of open handles was reached.
#define DFS_MAX_HANDLES_REACHED (dfs_err)13
///@brief Operation on requested file failed due to access restrictions.
#define DFS_UNAUTHORIZED_ACCESS (dfs_err)14
///@brief Attempted to access an invalid descriptor.
#define DFS_NVAL_DESCRIPTOR (dfs_err)15

//===File mode flags===
#define DFS_FILEM_READ (dfs_filem_flags)0x00000001
#define DFS_FILEM_WRITE (dfs_filem_flags)0x000000002
#define DFS_FILEM_RDWR (dfs_filem_flags)0x000000003
#define DFS_FILEM_SHARE_READ (dfs_filem_flags)0x00000004
#define DFS_FILEM_SHARE_WRITE (dfs_filem_flags)0x00000008
#define DFS_FILEM_SHARE_RDWR (dfs_filem_flags)0x0000000C



/**
 * @brief Initializes a partition on the given file/device
 * 
 * @param device Path to the file/device to use
 * @param dataSize The size of the data space to allocate
 * @return int containing the error code for the operation
 */
dfs_err dfs_pcreate(const char *device, size_t total_size);
/**
 * @brief Opens an existing partition from a file/device
 * 
 * @param device Path to the file/device to use
 * @param pt_handle Pointer to a partition handle pointer
 * @return int containing the error code for the operation
 */
dfs_err dfs_popen(const char *device, dfs_partition **pt);
/**
 * @brief Closes an open partition, releasing all associated resources
 * 
 * @param pt_handle Pointer to a partition handle
 * @return int containing the error code for the operation
 */
dfs_err dfs_pclose(dfs_partition *pt_handle);

/**
 * @brief Creates an empty directory at the specified path
 * 
 * @param pt Pointer to a partition handle to be used
 * @param path Path of the directory to be created
 * @return int containing the error code for the operation
 */
dfs_err dfs_dcreate(dfs_partition *pt, const char *path);
/**
 * @brief Create an empty file at the specified path
 * 
 * @param pt Pointer to a partition handle to be used
 * @param path Path of the directory to be created
 * @return int containing the error code for the operation
 */
dfs_err dfs_fcreate(dfs_partition *pt, const char *path);

/**
 * @brief Opens an existing file at the specified path
 * 
 * @param pt Pointer to a partition handle to be used
 * @param path Path of the directory to be created
 * @param fsHandle Pointer to the file handle pointer to populate
 * @return int containing the error code for the operation
 */
dfs_err dfs_fopen(dfs_partition *pt, const char *path, const dfs_filem_flags flags, int *descriptor);
/**
 * @brief Closes an open file handle and flushes and buffered changes
 * 
 * @param fs Pointer to the file handle to be closed
 * @return int containing the error code for the operation
 */
dfs_err dfs_fclose(dfs_partition *pt, const int descriptor);

/**
 * @brief Writes a block of data to a stream
 * 
 * @param buffer Pointer to the buffer to write the data from
 * @param len Length in bytes of the data to be written
 * @param fs Pointer to the file handle to write to
 * @param written Referenced variable will be set to the actual number of bytes written
 * @return int containing the error code for the operation
 */
dfs_err dfs_fwrite(dfs_partition *pt, const int descriptor, const void *buffer, const size_t len, size_t *written);
/**
 * @brief Reads a block of data from a stream
 * 
 * @param buffer Pointer to a buffer to read the data to
 * @param len Length in bytes of the data to be read
 * @param fs Pointer to the file handle to read from
 * @param written Referenced variable will be set to the actual number of bytes read
 * @return int containing the error code for the operation
 */
dfs_err dfs_fread(dfs_partition *pt, const int descriptor, const void *buffer, const size_t len, size_t *read);
/**
 * @brief Sets the stream position
 * 
 * @param pos The new position to set the stream to
 * @param fs Pointer to the file handle to modify
 * @return int containing the error code for the operation
 */
dfs_err dfs_fseek(dfs_partition *pt, const int descriptor, const size_t pos /*TODO: WHENCE and return position*/);
#endif
