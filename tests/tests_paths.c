#include <stdio.h>

#include "framework/unity.h"
#include "framework/unity_fixture.h"

#include "../src/paths.h"



TEST_GROUP(path_good);

TEST_SETUP(path_good) { }

TEST_TEAR_DOWN(path_good) { }

TEST(path_good, combine)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	char buff[32]; buff[31] = '\0';

	TEST_ASSERT_EQUAL_STRING("./Joel/Leal", dfs_path_combine(buff, "./Joel", "Leal"));
	TEST_ASSERT_EQUAL_STRING("./Joel/Leal", dfs_path_combine(buff, "./Joel/", "Leal"));
	TEST_ASSERT_EQUAL_STRING("./Joel/Leal", dfs_path_combine(buff, "./Joel", "/Leal"));
	TEST_ASSERT_EQUAL_STRING("./Joel/Leal", dfs_path_combine(buff, "./Joel/", "/Leal"));
}

TEST(path_good, get_parent)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	char buff[32]; buff[31] = '\0';

	TEST_ASSERT_EQUAL_STRING("./Joel/asmr", dfs_path_get_parent(buff, "./Joel/asmr/bdsm/"));
	TEST_ASSERT_EQUAL_STRING("./Joel", dfs_path_get_parent(buff, "./Joel/asmr"));
	TEST_ASSERT_EQUAL_STRING("Joel", dfs_path_get_parent(buff, "Joel/Joel"));
	TEST_ASSERT_EQUAL_STRING("", dfs_path_get_parent(buff, "Joel"));
}

TEST(path_good, get_name)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	char buff[32]; buff[31] = '\0';

	TEST_ASSERT_EQUAL_STRING("bdsm", dfs_path_get_name(buff, "./Joel/asmr/bdsm/"));
	TEST_ASSERT_EQUAL_STRING("asmr", dfs_path_get_name(buff, "./Joel/asmr"));
	TEST_ASSERT_EQUAL_STRING("Joel", dfs_path_get_name(buff, "Joel/Joel"));
	TEST_ASSERT_EQUAL_STRING("Joel", dfs_path_get_name(buff, "Joel"));
}

TEST_GROUP_RUNNER(path_good)
{
	RUN_TEST_CASE(path_good, combine);
	RUN_TEST_CASE(path_good, get_parent);
	RUN_TEST_CASE(path_good, get_name);
}
