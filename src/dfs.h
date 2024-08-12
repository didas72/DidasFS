//dfs.h - Defines public interface of the file system

#ifndef DIDASFS_H
#define DIDASFS_H

#include <stddef.h>
#include <stdint.h>

#include "paths.h"


//===Types===
///@brief Holds an error code
typedef int dfs_err;
/// @brief Holds file creation flags
typedef uint16_t dfs_filec_flags;
///@brief Holds file mode flags
typedef uint32_t dfs_filem_flags;
///@brief Represents a partition handle
typedef struct dfs_partition dfs_partition;


//===Structs===
typedef struct
{
	bool dir;
	size_t length;
	char name[MAX_PATH];
} dfs_entry;


//===Constants===
#define DFS_MAX_HANDLES 64


//===Error codes===
///@brief Function has not been (fully) implemented
#define DFS_NOT_IMPLEMENTED (dfs_err)-1
///@brief Operation performed successfully
#define DFS_SUCCESS (dfs_err)0
///@brief Function failed for an unspecified reason
#define DFS_FAIL (dfs_err)1
///@brief Function was given invalid arguments
#define DFS_NVAL_ARGS (dfs_err)2
///@brief Function was unable to open the requested underlying file/device
#define DFS_FAILED_DEVICE_OPEN (dfs_err)3
///@brief [UNUSED] Function failed to reserve the required file/device space
#define DFS_FAILED_SPACE_RESERVE (dfs_err)4
///@brief Failed to write to the underlying file/device
#define DFS_FAILED_DEVICE_WRITE (dfs_err)5
///@brief Failed to read from the underlying file/device
#define DFS_FAILED_DEVICE_READ (dfs_err)6
///@brief Failed to allocate the required memory
#define DFS_FAILED_ALLOC (dfs_err)7
///@brief Irrecoverable partition corruption was found
#define DFS_CORRUPTED_PARTITION (dfs_err)8
///@brief Function encountered invalid flags
#define DFS_NVAL_FLAGS (dfs_err)9
///@brief Function failed to find part of the requested path
#define DFS_PATH_NOT_FOUND (dfs_err)10
///@brief Function failed to allocate space in the partition
#define DFS_NO_SPACE (dfs_err)11
///@brief Function received an invalid seek position
#define DFS_NVAL_SEEK (dfs_err)12
///@brief Failed to open file because maximum number of open handles was reached
#define DFS_MAX_HANDLES_REACHED (dfs_err)13
///@brief Operation on requested file failed due to access restrictions
#define DFS_UNAUTHORIZED_ACCESS (dfs_err)14
///@brief Attempted to access an invalid descriptor
#define DFS_NVAL_DESCRIPTOR (dfs_err)15
///@brief Attempted to create an object that already exists
#define DFS_ALREADY_EXISTS (dfs_err)16
///@brief Attempted to access an invalid path
#define DFS_NVAL_PATH (dfs_err)17

//===File mode flags===
#define DFS_FILEM_READ (dfs_filem_flags)0x00000001
#define DFS_FILEM_WRITE (dfs_filem_flags)0x000000002
#define DFS_FILEM_RDWR (DFS_FILEM_READ | DFS_FILEM_WRITE)
#define DFS_FILEM_SHARE_READ (dfs_filem_flags)0x00000004
#define DFS_FILEM_SHARE_WRITE (dfs_filem_flags)0x00000008
#define DFS_FILEM_SHARE_RDWR (DFS_FILEM_SHARE_READ | DFS_FILEM_SHARE_WRITE)

//===Logging levels===
#define DFS_LOG_NONE 0
#define DFS_LOG_ERROR 1
#define DFS_LOG_WARNING 2
#define DFS_LOG_DEBUG 3


//===Function declarations===
/**
 * @brief Changes the logging level for the library
 * 
 * @param level The logging level to be used
*/
void dfs_set_log_level(int level);

/**
 * @brief Initializes a partition on the given file/device
 * 
 * @param device Path to the file/device to use
 * @param total_size The maximum size the partition may have
 * @return int containing the error code for the operation
 */
dfs_err dfs_pcreate(const char *device, size_t total_size);
/**
 * @brief Opens an existing partition from a file/device
 * 
 * @param device Path to the file/device to use
 * @param pt Pointer to a partition handle pointer
 * @return int containing the error code for the operation
 */
dfs_err dfs_popen(const char *device, dfs_partition **pt);
/**
 * @brief Closes an open partition, releasing all associated resources
 * 
 * @param pt Pointer to a partition handle
 * @return int containing the error code for the operation
 */
dfs_err dfs_pclose(dfs_partition *pt);

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
 * @param path Path of the file to be created
 * @param flags File mode flags to be used
 * @param descriptor Pointer to the file descriptor to populate
 * @return int containing the error code for the operation
 */
dfs_err dfs_fopen(dfs_partition *pt, const char *path, const dfs_filem_flags flags, int *descriptor);
/**
 * @brief Closes an open file handle and flushes buffered changes
 * 
 * @param fs Pointer to the file handle to be closed
 * @param descriptor Pointer to the file descriptor to populate
 * @return int containing the error code for the operation
 */
dfs_err dfs_fclose(dfs_partition *pt, const int descriptor);

/**
 * @brief Writes a block of data to a file
 * 
 * @param pt Pointer to a partition handle to be used
 * @param descriptor Descriptor of the file to be written to
 * @param buffer Pointer to the buffer to write the data from
 * @param len Length in bytes of the data to be written
 * @param written Referenced variable will be set to the actual number of bytes written
 * @return int containing the error code for the operation
 */
dfs_err dfs_fwrite(dfs_partition *pt, const int descriptor, const void *buffer, const size_t len, size_t *written);
/**
 * @brief Reads a block of data from a file
 * 
 * @param pt Pointer to a partition handle to be used
 * @param descriptor Descriptor of the file to be read from
 * @param buffer Pointer to a buffer to read the data to
 * @param len Length in bytes of the data to be read
 * @param written Referenced variable will be set to the actual number of bytes read
 * @return int containing the error code for the operation
 */
dfs_err dfs_fread(dfs_partition *pt, const int descriptor, void *buffer, const size_t len, size_t *read);
/**
 * @brief Sets the stream position
 * 
 * @param pt Pointer to a partition handle to be used
 * @param descriptor Descriptor of the file to be read from
 * @param pos The new position to set the stream to
 * @return int containing the error code for the operation
 */
dfs_err dfs_fset_pos(dfs_partition *pt, const int descriptor, const size_t pos);
/**
 * @brief Gets the stream position
 * 
 * @param pt Pointer to a partition handle to be used
 * @param descriptor Descriptor of the file to be read from
 * @param pos Referenced variable will be set to the current stream position
 * @return dfs_err 
 */
dfs_err dfs_fget_pos(dfs_partition *pt, const int descriptor, size_t *pos);

/**
 * @brief Lists entries present in a given directory
 * 
 * @param pt Pointer to a partition handle to be used
 * @param path Path of the directory whose contents should be listed
 * @param capacity Maximum number of entries that may be stored in entries
 * @param entries Pointer to an array to store the found entries. Can be set to NULL, if capacity is zero
 * @param count Used to return the nunmber of entries available
*/
dfs_err dfs_dlist_entries(dfs_partition *pt, const char *path, size_t capacity, dfs_entry *entries, size_t *count);
#endif
