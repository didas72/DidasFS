//errUtils.h - Utility functions to handle errors

#ifndef ERRUTILS_H
#define ERRUTILS_H

#include <stdio.h>

#define ERR_MSG(fmt, ...) { fprintf(stderr, fmt, ##__VA_ARGS__); }

#define ERR_MSG_NULL_ARG(argname) "Argument '"#argname"' cannot be NULL.\n"
#define ERR_MSG_ALLOC_FAIL "Could not allocate required memory space.\n"
#define ERR_MSG_DEVICE_READ_FAIL "Could not read from device.\n"
#define ERR_MSG_DEVICE_WRITE_FAIL "Could not write to device.\n"
#define ERR_MSG_DEVICE_OPEN_FAIL "Could not open device stream.\n"

#define ERR_IF 					ERR_NZERO
#define ERR_IF_FREE1 			ERR_NZERO_FREE1
#define ERR_IF_FREE2 			ERR_NZERO_FREE2
#define ERR_IF_FREE3 			ERR_NZERO_FREE3
#define ERR_IF_CLEANUP 			ERR_NZERO_CLEANUP
#define ERR_IF_CLEANUP_FREE1 	ERR_NZERO_CLEANUP_FREE1

#define ERR(errCode, fmt, ...) { ERR_MSG(fmt, ##__VA_ARGS__); return errCode; }

#define ERR_NULL(errIfNull, errCode, fmt, ...) {\
	if (!errIfNull) { ERR_MSG(fmt, ##__VA_ARGS__); return errCode; } }
#define ERR_NULL_FREE1(errIfNull, errCode, toFree1, fmt, ...) {\
	if (!errIfNull) { ERR_MSG(fmt, ##__VA_ARGS__); free(toFree1); return errCode;}}
#define ERR_NULL_FREE2(errIfNull, errCode, toFree1, toFree2, fmt, ...) {\
	if (!errIfNull) { ERR_MSG(fmt, ##__VA_ARGS__); free(toFree1); free(toFree2); return errCode;}}
#define ERR_NULL_FREE3(errIfNull, errCode, toFree1, toFree2, toFree3, fmt, ...) {\
	if (!errIfNull) { ERR_MSG(fmt, ##__VA_ARGS__); free(toFree1); free(toFree2); free(toFree3); return errCode;}}
#define ERR_NULL_CLEANUP(errIfNull, errCode, cleanup, fmt, ...) {\
	if (!errIfNull) { ERR_MSG(fmt, ##__VA_ARGS__); cleanup; return errCode;}}
#define ERR_NULL_CLEANUP_FREE1(errIfNull, errCode, cleanup, toFree1, fmt, ...) {\
	if (!errIfNull) { ERR_MSG(fmt, ##__VA_ARGS__); cleanup; free(toFree1); return errCode;}}

#define ERR_NZERO(errIfNZero, errCode, fmt, ...) {\
	if (errIfNZero) return errCode; }
#define ERR_NZERO_FREE1(errIfNZero, errCode, toFree1, fmt, ...) {\
	if (errIfNZero) { ERR_MSG(fmt, ##__VA_ARGS__); free(toFree1); return errCode;}}
#define ERR_NZERO_FREE2(errIfNZero, errCode, toFree1, toFree2, fmt, ...) {\
	if (errIfNZero) { ERR_MSG(fmt, ##__VA_ARGS__); free(toFree1); free(toFree2); return errCode;}}
#define ERR_NZERO_FREE3(errIfNZero, errCode, toFree1, toFree2, toFree3, fmt, ...) {\
	if (errIfNZero) { ERR_MSG(fmt, ##__VA_ARGS__); free(toFree1); free(toFree2); free(toFree3); return errCode;}}
#define ERR_NZERO_CLEANUP(errIfNZero, errCode, cleanup, fmt, ...) {\
	if (errIfNZero) { ERR_MSG(fmt, ##__VA_ARGS__); cleanup; return errCode;}}
#define ERR_NZERO_CLEANUP_FREE1(errIfNZero, errCode, cleanup, toFree1, fmt, ...) {\
	if (errIfNZero) { ERR_MSG(fmt, ##__VA_ARGS__); cleanup; free(toFree1); return errCode;}}

#endif
