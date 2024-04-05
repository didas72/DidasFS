//err_utils.h - Utility functions to handle errors

#ifndef ERRUTILS_H
#define ERRUTILS_H

#include <stdio.h>

#define MSG(level, stream, fmt, ...) do { \
	if (log_level >= level) fprintf(stream, fmt, ##__VA_ARGS__); \
	} while(0)

#define ERR_MSG(fmt, ...) MSG(DFS_LOG_ERROR, stderr, "DFS_ERR: [%s:%d] " fmt, __FILE__, __LINE__, ##__VA_ARGS__)

#define WARN_MSG(fmt, ...) MSG(DFS_LOG_WARNING, stderr, "DFS_WARN: " fmt, ##__VA_ARGS__)

#define DEBUG_MSG(fmt, ...) MSG(DFS_LOG_DEBUG, stderr, "DFS_DEBUG: " fmt, ##__VA_ARGS__)

#define ERR_MSG_NULL_ARG(argname) "Argument '"#argname"' cannot be NULL.\n"
#define ERR_MSG_ALLOC_FAIL "Could not allocate required memory.\n"
#define ERR_MSG_DEVICE_READ_FAIL "Could not read from device.\n"
#define ERR_MSG_DEVICE_WRITE_FAIL "Could not write to device.\n"
#define ERR_MSG_DEVICE_OPEN_FAIL "Could not open device stream.\n"
#define ERR_MSG_UNAUTHORIZED_ACCESS(operation, path) "Could not " operation " file '%s' due to access restrictions.\n", path
#define ERR_MSG_NVAL_DESCRIPTOR(operation, descriptor) "Cannot " operation " invalid descriptor '%d'.\n", descriptor
#define ERR_MSG_HANDLE_FETCH_FAIL(descriptor) "Could not fetch handle for descriptor '%d'.\n", descriptor
#define ERR_MSG_EMPTY_PATH(operation) "Cannot " operation " an object of empty path.\n"
#define ERR_MSG_NVAL_FLAGS(operation) "The provided flags are not valid for " operation ".\n"
#define ERR_MSG_ALREADY_EXISTS(path) "Object of path '%s' already exists.\n", path

#define ERR(errCode, fmt, ...) do { ERR_MSG(fmt, ##__VA_ARGS__); return errCode; } while(0)

#define ERR_IF(errIfNZero, errCode, fmt, ...) \
	ERR_NZERO(errIfNZero, errCode, fmt, ##__VA_ARGS__)
#define ERR_IF_FREE1(errIfNZero, errCode, toFree1, fmt, ...) \
	ERR_NZERO_FREE1(errIfNZero, errCode, toFree1, fmt, ##__VA_ARGS__)
#define ERR_IF_FREE2(errIfNZero, errCode, toFree1, toFree2, fmt, ...) \
	ERR_NZERO_FREE2(errIfNZero, errCode, toFree1, toFree2, fmt, ##__VA_ARGS__)
#define ERR_IF_FREE3(errIfNZero, errCode, toFree1, toFree2, toFree3, fmt, ...) \
	ERR_NZERO_FREE3(errIfNZero, errCode, toFree1, toFree2, toFree3, fmt, ##__VA_ARGS__)
#define ERR_IF_CLEANUP(errIfNZero, errCode, cleanup, fmt, ...) \
	ERR_NZERO_CLEANUP(errIfNZero, errCode, cleanup, fmt, ##__VA_ARGS__)
#define ERR_IF_CLEANUP_FREE1(errIfNZero, errCode, cleanup, toFree1, fmt, ...) \
	ERR_NZERO_CLEANUP_FREE1(errIfNZero, errCode, cleanup, toFree1, fmt, ##__VA_ARGS__)

#define ERR_NULL(errIfNull, errCode, fmt, ...) do {\
	if (!errIfNull) { ERR_MSG(fmt, ##__VA_ARGS__); return errCode; } } while(0)
#define ERR_NULL_FREE1(errIfNull, errCode, toFree1, fmt, ...) do {\
	if (!errIfNull) { ERR_MSG(fmt, ##__VA_ARGS__); free(toFree1); return errCode;}} while(0)
#define ERR_NULL_FREE2(errIfNull, errCode, toFree1, toFree2, fmt, ...) do {\
	if (!errIfNull) { ERR_MSG(fmt, ##__VA_ARGS__); free(toFree1); free(toFree2); return errCode;}} while(0)
#define ERR_NULL_FREE3(errIfNull, errCode, toFree1, toFree2, toFree3, fmt, ...) do {\
	if (!errIfNull) { ERR_MSG(fmt, ##__VA_ARGS__); free(toFree1); free(toFree2); free(toFree3); return errCode;}} while(0)
#define ERR_NULL_CLEANUP(errIfNull, errCode, cleanup, fmt, ...) do {\
	if (!errIfNull) { ERR_MSG(fmt, ##__VA_ARGS__); cleanup; return errCode;}} while(0)
#define ERR_NULL_CLEANUP_FREE1(errIfNull, errCode, cleanup, toFree1, fmt, ...) do {\
	if (!errIfNull) { ERR_MSG(fmt, ##__VA_ARGS__); cleanup; free(toFree1); return errCode;}} while(0)

#define ERR_NZERO(errIfNZero, errCode, fmt, ...) do {\
	if (errIfNZero) { ERR_MSG(fmt, ##__VA_ARGS__); return errCode; } } while(0)
#define ERR_NZERO_FREE1(errIfNZero, errCode, toFree1, fmt, ...) do {\
	if (errIfNZero) { ERR_MSG(fmt, ##__VA_ARGS__); free(toFree1); return errCode;}} while(0)
#define ERR_NZERO_FREE2(errIfNZero, errCode, toFree1, toFree2, fmt, ...) do {\
	if (errIfNZero) { ERR_MSG(fmt, ##__VA_ARGS__); free(toFree1); free(toFree2); return errCode;}} while(0)
#define ERR_NZERO_FREE3(errIfNZero, errCode, toFree1, toFree2, toFree3, fmt, ...) do {\
	if (errIfNZero) { ERR_MSG(fmt, ##__VA_ARGS__); free(toFree1); free(toFree2); free(toFree3); return errCode;}} while(0)
#define ERR_NZERO_CLEANUP(errIfNZero, errCode, cleanup, fmt, ...) do {\
	if (errIfNZero) { ERR_MSG(fmt, ##__VA_ARGS__); cleanup; return errCode;}} while(0)
#define ERR_NZERO_CLEANUP_FREE1(errIfNZero, errCode, cleanup, toFree1, fmt, ...) do {\
	if (errIfNZero) { ERR_MSG(fmt, ##__VA_ARGS__); cleanup; free(toFree1); return errCode;}} while(0)

#endif
