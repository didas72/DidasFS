//dPaths.c - Implements DPaths.c

#include "paths.h"

#include <stdlib.h>
#include <string.h>

#include "err_utils.h"



//======================
//= Internal functions =
//======================
static const char *get_first_not_tip(const char *src, char ch)
{
	size_t len = strlen(src);

	if (len < 2) //len 0 or 1 = empty or tip only
		return NULL;

	size_t head = 0;

	while (head < len)
	{
		if (src[head] == ch)
			return &src[head];

		++head;
	}

	return NULL;
}

static const char *get_last_not_tip(const char *src, char ch)
{
	size_t len = strlen(src);

	if (len < 2) //len 0 or 1 = empty or tip only
		return NULL;

	const char *head = &src[len - 2];

	while (head >= src)
	{
		if (*head == ch)
			return head;

		--head;
	}

	return NULL;
}




//============================
//= Function implementations =
//============================
char dfs_path_ends_in_separator(const char *path)
{
	if (path == NULL || strlen(path) == 0)
		return 0;

	return path[strlen(path) - 1] == DIR_SEPARATOR_CH;

}

char *dfs_path_combine(char* destination, const char *path1, const char *path2)
{
	if (path1 == NULL && path2 == NULL)
		strcpy(destination, "");
	else if (path2 == NULL || strlen(path2) == 0)
		strcpy(destination, path1);
	else if (path1 == NULL || strlen(path1) == 0)
		strcpy(destination, path2);
	else
	{
		strcpy(destination, path1);

		char* last_char = &destination[strlen(path1) - 1];

		//Only add separator neither path1 nor path2 have it
		if (*last_char != DIR_SEPARATOR_CH && *path2 != DIR_SEPARATOR_CH)
			*(++last_char) = DIR_SEPARATOR_CH;

		//Strip separator if both path1 and path2 have it
		if (*last_char == DIR_SEPARATOR_CH && *path2 == DIR_SEPARATOR_CH)
			path2++;

		strcpy(++last_char, path2);
	}

	return destination;
}

char *dfs_path_get_parent(char *destination, const char *path)
{
	if (path == NULL || strlen(path) == 0)
		strcpy(destination, "");
	else
	{
		const char *sep = get_last_not_tip(path, DIR_SEPARATOR_CH);

		if (sep == NULL)
		{
			strcpy(destination, "");
			return destination;
		}

		size_t len = sep - path;
		strncpy(destination, path, len);
		destination[len] = '\0';
	}

	return destination;
}

char *dfs_path_get_name(char *destination, const char *path)
{
	if (path == NULL || strlen(path) == 0)
		strcpy(destination, "");
	else
	{
		const char *sep = get_last_not_tip(path, DIR_SEPARATOR_CH);

		if (sep == NULL)
		{
			strcpy(destination, path);
			return destination;
		}

		strcpy(destination, ++sep);

		//Strip last separator if present
		if (dfs_path_ends_in_separator(destination))
			destination[strlen(destination) - 1] = '\0';
	}

	return destination;
}

char *dfs_path_get_root(char *destination, const char *path)
{
	if (path == NULL || strlen(path) == 0)
		strcpy(destination, "");
	else
	{
		const char *sep = get_first_not_tip(path, DIR_SEPARATOR_CH);

		if (sep == NULL)
		{
			strcpy(destination, "");
			return destination;
		}

		size_t len = sep - path;
		strncpy(destination, path, len);
		destination[len] = '\0';

		//Strip last separator if present
		if (dfs_path_ends_in_separator(destination))
			destination[len - 1] = '\0';
	}

	return destination;
}

char *dfs_path_get_tail(char *destination, const char *path)
{
	if (path == NULL || strlen(path) == 0)
		strcpy(destination, "");
	else
	{
		const char *sep = get_first_not_tip(path, DIR_SEPARATOR_CH);

		if (sep == NULL)
		{
			strcpy(destination, path);
			return destination;
		}

		strcpy(destination, ++sep);
	}

	return destination;
}

bool dfs_path_is_empty(const char* path)
{
	return !strlen(path);
}
