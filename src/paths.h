//dPaths.h - Defines public interface of path handling functions

#ifndef DPATHS_H
#define DPATHS_H

#include <stdbool.h>

#define DIR_SEPARATOR_CH '/'
#define MAX_PATH_NAME 20
#define MAX_PATH 1024

/**
 * @brief Returns a value that indicates whether ends in a directory separator
 * 
 * @param path The path to be checked
 * @return 0 if the path does not end in a separator, non-zero otherwise
 */
char DPathEndsInSeparator(const char *path);
/** 
 * @brief Combines two path strings into a single one
 * 
 * @param destination Pointer to the destination array where the content is to be copied
 * @param path1 First path to combine
 * @param path2 Second path to combine
 * @return Destination is returned
 */
char *DPathCombine(char *destination, const char *path1, const char *path2);
/** 
 * @brief Gets parent directory from path
 * 
 * @param destination Pointer to the destination array where the content is to be copied
 * @param path The path to examine
 * @return Destination is returned
 */
char *DPathGetParent(char *destination, const char *path);
/** 
 * @brief Gets file/directory name from path
 * 
 * @param destination Pointer to the destination array where the content is to be copied
 * @param path The path to examine
 * @return Destination is returned
 */
char *DPathGetName(char *destination, const char *path);

char *DPathGetRoot(char *destination, const char *path);
char *DPathGetTail(char *destination, const char *path);
bool DPathIsEmpty(const char *path);

#endif
