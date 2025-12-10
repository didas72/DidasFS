#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "framework/unity.h"
#include "framework/unity_fixture.h"

#include "mocks.h"
#include "mock_utils.h"
#include "mocks_interface.h"

#include "../src/dfs.h"
#include "../src/dfs_structures.h"


static dfs_err err;
static dfs_partition *pt;


TEST_GROUP(partition_good);

TEST_SETUP(partition_good)
{
	mock_setup();
}

TEST_TEAR_DOWN(partition_good) { }

TEST(partition_good, create_partition)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	size_t avail_size = 1 << 20; //1M
	char *device = "./test_create_partition.hex";

	err = dfs_pcreate(device, avail_size);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);

	//TODO: Max size
}

TEST(partition_good, open_close_partition)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	size_t avail_size = 1 << 20; //1M
	char *device = "./test_open_close_partition.hex";
	dfs_pcreate(device, avail_size);

	err = dfs_popen(device, &pt);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);

	err = dfs_pclose(pt);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
}

TEST_GROUP_RUNNER(partition_good)
{
	RUN_TEST_CASE(partition_good, create_partition);
	RUN_TEST_CASE(partition_good, open_close_partition);
}



// =======================
// ===== ERROR TESTS =====
// =======================



TEST_GROUP(partition_err);

TEST_SETUP(partition_err)
{
	mock_setup();
}

TEST_TEAR_DOWN(partition_err) { }

TEST(partition_err, create_partition_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	size_t avail_size = 1 << 20; //1M
	char *device = "./test_create_partition_erros.hex";

	err = dfs_pcreate(NULL, avail_size);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_pcreate accepted a NULL device.");

	err = dfs_pcreate(device, 0);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_pcreate accepted 0 as available size.");

	err = dfs_pcreate(device, BLOCK_SIZE + SECTOR_SIZE - 1);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_pcreate accepted a value too small for available size.");

	err = dfs_pcreate(device, BLOCK_SIZE + SECTOR_SIZE);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_SUCCESS, err, "dfs_pcreate rejected the minimum value for available size.");

	//TODO: Larger than max size
}

TEST(partition_err, open_close_partition_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	char *device = "./test_open_close_partition_erros.hex";
	char *nval_device = "./invalid_device.hex";

	err = dfs_popen(NULL, &pt);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_popen accepted a NULL device.");

	err = dfs_popen(device, NULL);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_popen accepted a NULL partition pointer pointer.");

	err = dfs_popen(nval_device, &pt);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_FAILED_DEVICE_OPEN, err, "dfs_popen accepted an invalid device.");

	err = dfs_pclose(NULL);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_pclose accepted a NULL partition pointer.");
}

TEST(partition_err, open_corrupt_partition_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	char *device = "./test_open_corrupt_partition_errors.hex";
	char zero = 0;

	//Generate corrupted partition
	int fd = open(device, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	lseek(fd, 0x100000 - 1, SEEK_SET);
	write(fd, &zero, 1);
	close(fd);

	err = dfs_popen(device, &pt);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_CORRUPTED_PARTITION, err, "dfs_popen accepted a corrupted partition.");
}

TEST_GROUP_RUNNER(partition_err)
{
	RUN_TEST_CASE(partition_err, create_partition_errors);
	RUN_TEST_CASE(partition_err, open_close_partition_errors);
	RUN_TEST_CASE(partition_err, open_corrupt_partition_errors);
}
