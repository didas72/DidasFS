//errUtils.h - Utility functions to handle errors

#ifndef ERRUTILS_H
#define ERRUTILS_H

#define ERR_IF 					ERR_NZERO
#define ERR_IF_FREE1 			ERR_NZERO_FREE1
#define ERR_IF_FREE2 			ERR_NZERO_FREE2
#define ERR_IF_FREE3 			ERR_NZERO_FREE3
#define ERR_IF_CLEANUP 			ERR_NZERO_CLEANUP
#define ERR_IF_CLEANUP_FREE1 	ERR_NZERO_CLEANUP_FREE1

#define ERR_NULL(errIfNull, errCode) {\
	if (!errIfNull) return errCode; }
#define ERR_NULL_FREE1(errIfNull, errCode, toFree1) {\
	if (!errIfNull) { free(toFree1); return errCode;}}
#define ERR_NULL_FREE2(errIfNull, errCode, toFree1, toFree2) {\
	if (!errIfNull) { free(toFree1); free(toFree2); return errCode;}}
#define ERR_NULL_FREE3(errIfNull, errCode, toFree1, toFree2, toFree3) {\
	if (!errIfNull) { free(toFree1); free(toFree2); free(toFree3); return errCode;}}
#define ERR_NULL_CLEANUP(errIfNull, errCode, cleanup) {\
	if (!errIfNull) { cleanup; return errCode;}}
#define ERR_NULL_CLEANUP_FREE1(errIfNull, errCode, cleanup, toFree1) {\
	if (!errIfNull) { cleanup; free(toFree1); return errCode;}}

#define ERR_NZERO(errIfNZero, errCode) {\
	if (errIfNZero) return errCode; }
#define ERR_NZERO_FREE1(errIfNZero, errCode, toFree1) {\
	if (errIfNZero) { free(toFree1); return errCode;}}
#define ERR_NZERO_FREE2(errIfNZero, errCode, toFree1, toFree2) {\
	if (errIfNZero) { free(toFree1); free(toFree2); return errCode;}}
#define ERR_NZERO_FREE3(errIfNZero, errCode, toFree1, toFree2, toFree3) {\
	if (errIfNZero) { free(toFree1); free(toFree2); free(toFree3); return errCode;}}
#define ERR_NZERO_CLEANUP(errIfNZero, errCode, cleanup) {\
	if (errIfNZero) { cleanup; return errCode;}}
#define ERR_NZERO_CLEANUP_FREE1(errIfNZero, errCode, cleanup, toFree1) {\
	if (errIfNZero) { cleanup; free(toFree1); return errCode;}}

#endif
