#ifndef TESTS_MOCKS_H
#define TESTS_MOCKS_H

#include <stddef.h>
#include <sys/types.h>

#ifdef MOCK_DEVICE
#define MAX_FILES 32

extern size_t device_size_limit;
int ram_open(const char *pathname, int flags, ...);
int ram_close(int fd);
ssize_t ram_read(int fd, void *buf, size_t count);
ssize_t ram_write(int fd, void *buf, size_t count);
off_t ram_lseek(int fd, off_t offset, int whence);
void ram_reset_files(char do_free);
#endif

#endif
