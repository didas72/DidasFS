//mocks_interface.h - Forces the usage of mocks on the files that include this header

#ifndef TESTS_MOCKS_INTERFACE_H
#define TESTS_MOCKS_INTERFACE_H

#ifdef MOCK_DEVICE
#define open(pathname, flags...) ram_open(pathname, flags)
#define close(fd) ram_close(fd)
#define read(fd, buf, count) ram_read(fd, buf, count)
#define write(fd, buf, count) ram_write(fd, buf, count)
#define lseek(fd, offset, whence) ram_lseek(fd, offset, whence)
#endif

#endif
