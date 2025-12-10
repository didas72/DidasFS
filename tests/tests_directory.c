#include <stdio.h>

#include "framework/unity.h"
#include "framework/unity_fixture.h"

#include "mocks.h"
#include "mock_utils.h"
#include "mocks_interface.h"

#include "../src/dfs.h"


TEST_GROUP(directory_good);

TEST_SETUP(directory_good)
{
	mock_setup();

	size_t avail_size = 1 << 20; //1M
	char *device = "./test_directories_good.hex";

	dfs_pcreate(device, avail_size);
}

TEST_TEAR_DOWN(directory_good) { }

TEST(directory_good, create_directory)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_directories_good.hex";

	dfs_popen(device, &pt);

	err = dfs_dcreate(pt, "test dir");
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);

	err = dfs_dcreate(pt, "im at root");
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);

	dfs_pclose(pt);
}

TEST(directory_good, create_directory_nested)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_directories_good.hex";

	dfs_popen(device, &pt);
	dfs_dcreate(pt, "test dir");

	err = dfs_dcreate(pt, "test dir/more test dirs");
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);

	err = dfs_dcreate(pt, "test dir/testing...");
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);

	err = dfs_dcreate(pt, "test dir/more inside");
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);

	err = dfs_dcreate(pt, "test dir/more inside/im here");
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);

	dfs_pclose(pt);
}

TEST_GROUP_RUNNER(directory_good)
{
	RUN_TEST_CASE(directory_good, create_directory);
	RUN_TEST_CASE(directory_good, create_directory_nested);
}



// =======================
// ===== ERROR TESTS =====
// =======================



TEST_GROUP(directory_err);

TEST_SETUP(directory_err)
{
	mock_setup();

	size_t avail_size = 1 << 20; //1M
	char *device = "./test_directories_errors.hex";

	dfs_pcreate(device, avail_size);
}

TEST_TEAR_DOWN(directory_err) { }

TEST(directory_err, null_args_directories_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_directories_errors.hex";

	dfs_popen(device, &pt);

	err = dfs_dcreate(pt, NULL);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_dcreate accepted a NULL path.");
}

TEST(directory_err, duplicated_directories_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_directories_errors.hex";

	dfs_popen(device, &pt);
	dfs_dcreate(pt, "dup");

	err = dfs_dcreate(pt, "dup");
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_ALREADY_EXISTS, err, "dfs_dcreate created a duplicated directory.");
}

TEST(directory_err, empty_name_directories_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_directories_errors.hex";

	dfs_popen(device, &pt);

	err = dfs_dcreate(pt, "");
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_PATH, err, "dfs_dcreate accepted an empty root directory name.");
}

TEST(directory_err, object_inside_files_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_directories_errors.hex";

	dfs_popen(device, &pt);
	dfs_fcreate(pt, "file_not_dir");

	err = dfs_dcreate(pt, "file_not_dir/file");
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_PATH, err, "dfs_dcreate allowed the creation of directory under a file.");
}

TEST_GROUP_RUNNER(directory_err)
{
	RUN_TEST_CASE(directory_err, null_args_directories_errors);
	RUN_TEST_CASE(directory_err, duplicated_directories_errors);
	RUN_TEST_CASE(directory_err, empty_name_directories_errors);
	RUN_TEST_CASE(directory_err, object_inside_files_errors);
}
