//tests.c - Tests

#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <string.h>

#include "framework/unity.h"
#include "framework/unity_fixture.h"
#include "mocks.h"
#include "mock_utils.h"
#include "mocks_interface.h"

#include "../src/dfs.h"
#include "../src/paths.h"

//For more thourough tests and for constants access
#include "../src/dfs_structures.h"
//#include "../src/dfs_internals.h"



static void RunAllTests(void)
{
	RUN_TEST_GROUP(path_good);
	RUN_TEST_GROUP(partition_good);
	RUN_TEST_GROUP(partition_err);
	RUN_TEST_GROUP(directory_good);
	RUN_TEST_GROUP(directory_err);
	RUN_TEST_GROUP(file_good);
	RUN_TEST_GROUP(file_err);
	RUN_TEST_GROUP(management_good);
	RUN_TEST_GROUP(mamagement_err);
}

int main(int argc, char *argv[])
{
	return UnityMain(argc, (const char**)argv, RunAllTests);
}
