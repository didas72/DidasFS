#include "mocks.h"

#ifdef TESTS_MOCKS_INTERFACE_H
#warning "Detected mock interface in mock implementation"
#undef open
#undef close
#undef read
#undef write
#undef lseek
#endif

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>

#ifdef MOCK_DEVICE

typedef struct mem_fd_t
{
	char *data;
	size_t length;
	size_t offset;
	char name[128]; //Pray its enough
} mem_fd_t;

size_t device_size_limit = ~0u;
static int fd_counter = 0;
static mem_fd_t files[MAX_FILES];

static int file_ensure_capacity(int fd, size_t capacity)
{
	if (files[fd].length >= capacity)
		return 0;

	if (capacity >= device_size_limit)
	{
		fprintf(stderr, "Attempted to grow device past size limit.\n");
		return -1;
	}

	size_t old_length = files[fd].length;

	while (files[fd].length < capacity)
		files[fd].length <<= 1;
	
	void *tmp = realloc(files[fd].data, files[fd].length);
	if (!tmp)
	{
		fprintf(stderr, "Failed to realloc to size %ld\n", files[fd].length);
		return -1;
	}
	files[fd].data = tmp;
	memset(&files[fd].data[old_length], 0, files[fd].length - old_length);
	return 0;
}

int ram_open(const char *pathname, int flags, ...)
{
	printf("Open with %s and %d\n", pathname, flags);
	//ignore varargs

	int fd = -1;

	for (int i = 0; i < MAX_FILES; i++)
	{
		if (strcmp(pathname, files[i].name))
			continue;
		
		fd = i;
		break;
	}

	if (fd == -1)
	{ //File needs creation
		if ((flags & O_CREAT) == 0)
		{
			errno = ENOENT;
			printf("Need O_CREAT\n");
			return -1;
		}
		if (fd_counter >= MAX_FILES)
		{
			fprintf(stderr, "Reached max file cap\n");
			return -1;
		}
		fd = fd_counter++;
		strcpy(files[fd].name, pathname);
		files[fd].length = 1024;
		files[fd].data = malloc(files[fd].length);
	}

	files[fd].offset = 0;

	return fd;
}

int ram_close(int fd)
{ //Assume valid fd
	(void)fd;
	//Do nothing
	return 0;
}

ssize_t ram_read(int fd, void *buf, size_t count)
{ //Assume valid fd
	size_t max_count = files[fd].length - files[fd].offset;
	size_t actual_count = count > max_count ? max_count : count;
	memcpy(buf, &files[fd].data[files[fd].offset], actual_count);
	files[fd].offset += count;
	return actual_count;
}

ssize_t ram_write(int fd, void *buf, size_t count)
{ //Assume valid fd
	int ret = file_ensure_capacity(fd, files[fd].offset + count);
	if (ret)
		return ret;
	
	memcpy(&files[fd].data[files[fd].offset], buf, count);
	files[fd].offset += count;
	return count;
}

off_t ram_lseek(int fd, off_t offset, int whence)
{ //Assume valid fd
	if (whence == SEEK_SET)
	{
		int ret = file_ensure_capacity(fd, (size_t)offset);
		if (ret)
			return ret;
		files[fd].offset = (size_t)offset;
		return (off_t)files[fd].offset;
	}
	else if (whence == SEEK_CUR)
	{
		size_t off = files[fd].offset;
		if (offset >= 0) off += offset;
		else off -= (size_t)-offset;
		int ret = file_ensure_capacity(fd, off);
		if (ret)
			return ret;
		files[fd].offset = (size_t)offset;
		return (off_t)files[fd].offset;
	}
	else if (whence == SEEK_END)
	{
		fprintf(stderr, "ram_lseek with SEEK_END not implemented.\n");
		return -1;
	}

	return -1;
}

void ram_reset_files(char do_free)
{
	for (int i = 0; i < MAX_FILES && do_free; i++)
		free(files[i].data);
	memset(files, 0, sizeof(files));
	fd_counter = 0;
}
#endif
