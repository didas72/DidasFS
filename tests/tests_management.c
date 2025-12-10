#include <stdio.h>
#include <string.h>

#include "framework/unity.h"
#include "framework/unity_fixture.h"

#include "mocks.h"
#include "mock_utils.h"
#include "mocks_interface.h"

#include "../src/dfs.h"


TEST_GROUP(management_good);

TEST_SETUP(management_good)
{
	mock_setup();
	
	size_t avail_size = 1 << 20; //1M
	char *device = "./test_management_good.hex";

	dfs_pcreate(device, avail_size);
}

TEST_TEAR_DOWN(management_good) { }

TEST(management_good, list_entries)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_management_good.hex";

	size_t count;
	dfs_entry entries[16] = { 0 };
	dfs_popen(device, &pt);
	
	err = dfs_dlist_entries(pt, "", 0, NULL, &count);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(0, count);

	err = dfs_dlist_entries(pt, "", 16, entries, &count);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(0, count);

	dfs_fcreate(pt, "file1.test");
	dfs_dcreate(pt, "dir1");

	err = dfs_dlist_entries(pt, "", 0, NULL, &count);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(2, count);

	err = dfs_dlist_entries(pt, "", 16, entries, &count);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(2, count);
	//FIXME: Ordering is not required
	TEST_ASSERT_EQUAL_INT(false, entries[0].dir);
	TEST_ASSERT_EQUAL_INT(0, entries[0].length);
	TEST_ASSERT_EQUAL_STRING("file1.test", entries[0].name);
	TEST_ASSERT_EQUAL_INT(true, entries[1].dir);
	TEST_ASSERT_EQUAL_INT(0, entries[1].length);
	TEST_ASSERT_EQUAL_STRING("dir1", entries[1].name);

	err = dfs_dlist_entries(pt, "dir1", 0, NULL, &count);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(0, count);

	err = dfs_dlist_entries(pt, "dir1", 16, entries, &count);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(0, count);

	dfs_pclose(pt);
}

TEST_GROUP_RUNNER(management_good)
{
	RUN_TEST_CASE(management_good, list_entries);
}



// =======================
// ===== ERROR TESTS =====
// =======================



TEST_GROUP(mamagement_err);

TEST_SETUP(mamagement_err)
{
	mock_setup();
	
	size_t avail_size = 1 << 20; //1M
	char *device = "./test_management_errors.hex";

	dfs_pcreate(device, avail_size);
}

TEST_TEAR_DOWN(mamagement_err) { }

TEST(mamagement_err, list_entries_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_management_errors.hex";

	size_t count;
	dfs_entry entries[16] = { 0 };
	dfs_popen(device, &pt);

	err = dfs_dlist_entries(NULL, "", 16, entries, &count);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_list_entries accepted a NULL partition.");

	err = dfs_dlist_entries(pt, NULL, 16, entries, &count);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_dlist_entries accepted a NULL path.");

	err = dfs_dlist_entries(pt, "", 16, NULL, &count);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_dlist_entries accepted a NULL entries pointer with a non zero capacity.");

	dfs_pclose(pt);
}

TEST_GROUP_RUNNER(mamagement_err)
{
	RUN_TEST_CASE(mamagement_err, list_entries_errors);
}
