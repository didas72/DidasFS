//DPaths.c - Implements DPaths.c

#include "DPaths.h"

#include <stdlib.h>
#include <string.h>

#include "errUtils.h"



//================================
//= Internal function signatures =
//================================
static char *GetLastNotTip(char *src, char ch);
char *GetFirstNotTip(char *src, char ch);



//============================
//= Function implementations =
//============================
char DPathEndsInSeparator(char *path)
{
	if (path == NULL || strlen(path) == 0)
		return 0;

	return path[strlen(path) - 1] == DIR_SEPARATOR_CH;

}

char *DPathCombine(char* destination, char *path1, char *path2)
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

		char* lastChar = &destination[strlen(path1) - 1];

		//Only add separator neither path1 nor path2 have it
		if (*lastChar != DIR_SEPARATOR_CH && *path2 != DIR_SEPARATOR_CH)
			*(++lastChar) = DIR_SEPARATOR_CH;

		//Strip separator if both path1 and path2 have it
		if (*lastChar == DIR_SEPARATOR_CH && *path2 == DIR_SEPARATOR_CH)
			path2++;

		strcpy(++lastChar, path2);
	}

	return destination;
}

char *DPathGetParent(char *destination, char *path)
{
	if (path == NULL || strlen(path) == 0)
		strcpy(destination, "");
	else
	{
		char *sep = GetLastNotTip(path, DIR_SEPARATOR_CH);

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

char *DPathGetName(char *destination, char *path)
{
	if (path == NULL || strlen(path) == 0)
		strcpy(destination, "");
	else
	{
		char *sep = GetLastNotTip(path, DIR_SEPARATOR_CH);

		if (sep == NULL)
		{
			strcpy(destination, path);
			return destination;
		}

		strcpy(destination, ++sep);

		//Strip last separator if present
		if (DPathEndsInSeparator(destination))
			destination[strlen(destination) - 1] = '\0';
	}

	return destination;
}

char *DPathGetRoot(char *destination, char *path)
{
	if (path == NULL || strlen(path) == 0)
		strcpy(destination, "");
	else
	{
		char *sep = GetFirstNotTip(path, DIR_SEPARATOR_CH);

		if (sep == NULL)
		{
			strcpy(destination, "");
			return destination;
		}

		size_t len = sep - path;
		strncpy(destination, path, len);
		destination[len] = '\0';

		//Strip last separator if present
		if (DPathEndsInSeparator(destination))
			destination[len - 1] = '\0';
	}

	return destination;
}

char *DPathGetExeceptRoot(char *destination, char *path)
{
	if (path == NULL || strlen(path) == 0)
		strcpy(destination, "");
	else
	{
		char *sep = GetFirstNotTip(path, DIR_SEPARATOR_CH);

		if (sep == NULL)
		{
			strcpy(destination, path);
			return destination;
		}

		strcpy(destination, ++sep);
	}

	return destination;
}

//=====================================
//= Internal function implementations =
//=====================================
char *GetFirstNotTip(char *src, char ch)
{
	size_t len = strlen(src);

	if (len < 2) //len 0 or 1 = empty or tip only
		return NULL;

	int head = 0;

	while (head < len)
	{
		if (src[head] == ch)
			return &src[head];

		++head;
	}

	return NULL;
}

char *GetLastNotTip(char *src, char ch)
{
	size_t len = strlen(src);

	if (len < 2) //len 0 or 1 = empty or tip only
		return NULL;

	char *head = &src[len - 2];

	while (head >= src)
	{
		if (*head == ch)
			return head;

		--head;
	}

	return NULL;
}
