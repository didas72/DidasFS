# TODO list

DOCS: Finish doxygen for dPaths.h

REMINDER: Cleanup TODO in code

TEST: Fail conditions
TEST: Partition sizing

ADD FEATURE: ftell equivalent
ADD FEATURE: Delete file
ADD FEATURE: Delete directory (recursive opt)
ADD FEATURE: List files/directories
ADD FEATURE: Exists file/directory
ADD FEATURE: Copy file
ADD FEATURE: Move dile/dir
ADD FEATURE: Check total free space
ADD FEATURE: Verify and cleanup

ADD FEATURE: DPartition must track open file handles
ADD FEATURE: OpenFile must not allow opening of directories as files
ADD FEATURE: OpenFile must register file handle
ADD FEATURE: OpenFile must check for already open handles
ADD FEATURE: OpenFile must check file flags and match handle type
ADD FEATURE: CloseFile must remove file handle

PERFORMANCE: Make BlockMap changes buffered
PERFORMANCE: Make reads/writes buffered
PERFORMANCE: Remove system buffering
REMINDER: Ensure flushes when closing streams (both in FS and in system)

FIX: ?Needed? Replace all fread to work with 512 multiples for hardware devices
