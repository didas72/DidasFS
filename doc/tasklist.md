# TODO list

REWORK: Use descriptors and errno-type error system:

* Remove DFileStreams and switch to internal descriptors
* Change error system to errno-similar and use returns for descriptors
* DPartition must track open file handles
* dfopen must not allow opening of directories as files
* dfopen must register file handle
* dfopen must check for already open handles
* dfopen must check file flags and match handle type
* dfclose must remove file handle

ADD FEATURE: Implement target interfaces:

* IO interface: fopen, fread, fwrite, fclose, ftruncate, fsync (flush)
* Management interface: mkdir, rmdir, create (touch), rmfile (rm)

DOCS: Finish doxygen for dPaths.h

REMINDER: Cleanup TODO in code

TEST: Fail conditions
TEST: Partition sizing

ADD FEATURE: Implement higher level:

* List files/directories
* Exists file/directory
* Copy file
* Move dile/dir
* Check total free space
* Verify and cleanup FS

PERFORMANCE: Make BlockMap changes buffered
PERFORMANCE: Make reads/writes buffered
PERFORMANCE: Remove system buffering
REMINDER: Ensure flushes when closing streams (both in FS and in system)

ADD FEATURE: FS recovery

FIX: ?Needed? Replace all fread to work with 512 multiples for hardware devices
