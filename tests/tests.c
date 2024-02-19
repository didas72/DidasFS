//tests.c - Tests

#include <stdlib.h>

#include "framework/minunit.h"

#include "../src/dfs.h"
#include "../src/paths.h"

//For more thourough tests and for constants access
#include "../src/dfs_structures.h"
//#include "../src/dfs_internals.h"

//===Path functions===
MU_TEST(combine)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	char buff[32]; buff[31] = '\0';

	mu_assert_string_eq("./Joel/Leal", dfs_path_combine(buff, "./Joel", "Leal"));
	mu_assert_string_eq("./Joel/Leal", dfs_path_combine(buff, "./Joel/", "Leal"));
	mu_assert_string_eq("./Joel/Leal", dfs_path_combine(buff, "./Joel", "/Leal"));
	mu_assert_string_eq("./Joel/Leal", dfs_path_combine(buff, "./Joel/", "/Leal"));
}

MU_TEST(get_parent)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	char buff[32]; buff[31] = '\0';

	mu_assert_string_eq("./Joel/asmr", dfs_path_get_parent(buff, "./Joel/asmr/bdsm/"));
	mu_assert_string_eq("./Joel", dfs_path_get_parent(buff, "./Joel/asmr"));
	mu_assert_string_eq("Joel", dfs_path_get_parent(buff, "Joel/Joel"));
	mu_assert_string_eq("", dfs_path_get_parent(buff, "Joel"));
}

MU_TEST(get_name)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	char buff[32]; buff[31] = '\0';

	mu_assert_string_eq("bdsm", dfs_path_get_name(buff, "./Joel/asmr/bdsm/"));
	mu_assert_string_eq("asmr", dfs_path_get_name(buff, "./Joel/asmr"));
	mu_assert_string_eq("Joel", dfs_path_get_name(buff, "Joel/Joel"));
	mu_assert_string_eq("Joel", dfs_path_get_name(buff, "Joel"));
}

MU_TEST_SUITE(dfs_path_all)
{
	MU_RUN_TEST(combine);
	MU_RUN_TEST(get_parent);
	MU_RUN_TEST(get_name);
}


//===Partition functions===
MU_TEST(create_partition)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	size_t avail_size = 1 << 20; //1M
	char *device = "./test_create_partition.hex";

	err = dfs_pcreate(device, avail_size);
	mu_assert_int_eq(0, err);
}

MU_TEST(open_close_partition)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	size_t avail_size = 1 << 20; //1M
	char *device = "./test_open_close_partition.hex";

	err = dfs_pcreate(device, avail_size);
	mu_assert(err == 0, "Partition creation failed.");

	err = dfs_popen(device, &pt);
	mu_assert_int_eq(0, err);

	err = dfs_pclose(pt);
	mu_assert_int_eq(0, err);
}

MU_TEST_SUITE(dfs_partition_good)
{
	MU_RUN_TEST(create_partition);
	MU_RUN_TEST(open_close_partition);
}


//===Partition function errors===
MU_TEST(create_partition_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	size_t avail_size = 1 << 20; //1M
	char *device = "./test_create_partition_erros.hex";

	//Spotted and fixed
	err = dfs_pcreate(NULL, avail_size);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_pcreate accepted a NULL device.");

	err = dfs_pcreate(device, 0);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_pcreate accepted 0 as available size.");

	err = dfs_pcreate(device, BLOCK_SIZE + SECTOR_SIZE - 1);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_pcreate accepted a value too small for available size.");

	err = dfs_pcreate(device, BLOCK_SIZE + SECTOR_SIZE);
	mu_assert(err == DFS_SUCCESS, "dfs_pcreate rejected the minimum value for available size.");
}

MU_TEST(open_close_partition_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_open_close_partition_erros.hex";
	char *nval_device = "./invalid_device.hex";

	//Spotted and fixed
	err = dfs_popen(NULL, &pt);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_popen accepted a NULL device.");

	err = dfs_popen(device, NULL);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_popen accepted a NULL partition pointer pointer.");

	err = dfs_popen(nval_device, &pt);
	mu_assert(err == DFS_FAILED_DEVICE_OPEN, "dfs_popen accepted an invalid device.");

	err = dfs_pclose(NULL);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_pclose accepted a NULL partition pointer.");
}

MU_TEST(open_corrupt_partition_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_open_corrupt_partition_errors.hex";
	char zero = 0;

	//Generate corrupted partition
	FILE *file = fopen(device, "wb");
	fseek(file, 0x100000 - 1, SEEK_SET);
	fwrite(&zero, 1, 1, file);
	fclose(file);

	err = dfs_popen(device, &pt);
	mu_assert(err == DFS_CORRUPTED_PARTITION, "dfs_popen accepted a corrupted partition.");
}

MU_TEST_SUITE(dfs_partition_errors)
{
	MU_RUN_TEST(create_partition_errors);
	MU_RUN_TEST(open_close_partition_errors);
	MU_RUN_TEST(open_corrupt_partition_errors);
}


//===Directory functions===
MU_TEST(create_directory)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_directories_good.hex";

	dfs_popen(device, &pt);

	err = dfs_dcreate(pt, "test dir");
	mu_assert_int_eq(0, err);

	err = dfs_dcreate(pt, "im at root");
	mu_assert_int_eq(0, err);

	dfs_pclose(pt);
}

MU_TEST(create_directory_nested)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_directories_good.hex";

	dfs_popen(device, &pt);

	err = dfs_dcreate(pt, "test dir/more test dirs");
	mu_assert_int_eq(0, err);

	err = dfs_dcreate(pt, "test dir/testing...");
	mu_assert_int_eq(0, err);

	err = dfs_dcreate(pt, "test dir/more inside");
	mu_assert_int_eq(0, err);

	err = dfs_dcreate(pt, "test dir/more inside/im here");
	mu_assert_int_eq(0, err);

	dfs_pclose(pt);
}

MU_TEST_SUITE(dfs_directories_good)
{
	size_t avail_size = 1 << 20; //1M
	char *device = "./test_directories_good.hex";

	dfs_pcreate(device, avail_size);

	MU_RUN_TEST(create_directory);
	MU_RUN_TEST(create_directory_nested);
}

//===Directory function errors===
MU_TEST(duplicated_directories_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_directories_errors.hex";

	dfs_popen(device, &pt);
	dfs_dcreate(pt, "dup");

	err = dfs_dcreate(pt, "dup");
	mu_assert(err == DFS_ALREADY_EXISTS, "dfs_dcreate created a duplicated directory.");
}

MU_TEST(empty_name_directories_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_directories_errors.hex";

	dfs_popen(device, &pt);

	err = dfs_dcreate(pt, "");
	mu_assert(err == DFS_NVAL_PATH, "dfs_dcreate accepted an empty root directory name.");
}

MU_TEST_SUITE(dfs_directories_errors)
{
	size_t avail_size = 1 << 20; //1M
	char *device = "./test_directories_errors.hex";

	dfs_pcreate(device, avail_size);

	MU_RUN_TEST(duplicated_directories_errors);
	MU_RUN_TEST(empty_name_directories_errors);
}

//TODO: //===File functions===
//TODO: //===File function errors===


int main()
{
	MU_RUN_SUITE(dfs_path_all);
	MU_RUN_SUITE(dfs_partition_good);
	MU_RUN_SUITE(dfs_partition_errors);
	MU_RUN_SUITE(dfs_directories_good);
	MU_RUN_SUITE(dfs_directories_errors);
	MU_REPORT();
	
	return MU_EXIT_CODE;
}
