//DPaths.h - Defines public interface of path handling functions

#ifndef DPATHS_H
#define DPATHS_H

#define DIR_SEPARATOR_CH '/'

/// @brief Returns a value that indicates whether ends in a directory separator
/// @param path The path to be checked
/// @return 0 if the path does not end in a separator, non-zero otherwise
char DPathEndsInSeparator(char *path);
/// @brief Combines two path strings into a single one
/// @param destination Pointer to the destination array where the content is to be copied
/// @param path1 First path to combine
/// @param path2 Second path to combine
/// @return Destination is returned
char *DPathCombine(char *destination, char *path1, char *path2);
/// @brief Gets parent directory from path
/// @param destination Pointer to the destination array where the content is to be copied
/// @param path The path to examine
/// @return Destination is returned
char *DPathGetParent(char *destination, char *path);
/// @brief Gets file/directory name from path
/// @param destination Pointer to the destination array where the content is to be copied
/// @param path The path to examine
/// @return Destination is returned
char *DPathGetName(char *destination, char *path);

char *DPathGetRoot(char *destination, char *path);
char *DPathGetExeceptRoot(char *destination, char *path);

#endif
