//DPaths.c - Tests

#include <stdlib.h>

#include "framework/minunit.h"

#include "../src/dfs.h"
#include "../src/paths.h"

//For more thourough tests and for constants access
#include "../src/dfs_structures.h"

//===Path functions===
MU_TEST(combine)
{
	char buff[32]; buff[31] = '\0';

	mu_assert_string_eq("./Joel/Leal", dfs_path_combine(buff, "./Joel", "Leal"));
	mu_assert_string_eq("./Joel/Leal", dfs_path_combine(buff, "./Joel/", "Leal"));
	mu_assert_string_eq("./Joel/Leal", dfs_path_combine(buff, "./Joel", "/Leal"));
	mu_assert_string_eq("./Joel/Leal", dfs_path_combine(buff, "./Joel/", "/Leal"));
}

MU_TEST(get_parent)
{
	char buff[32]; buff[31] = '\0';

	mu_assert_string_eq("./Joel/asmr", dfs_path_get_parent(buff, "./Joel/asmr/bdsm/"));
	mu_assert_string_eq("./Joel", dfs_path_get_parent(buff, "./Joel/asmr"));
	mu_assert_string_eq("Joel", dfs_path_get_parent(buff, "Joel/Joel"));
	mu_assert_string_eq("", dfs_path_get_parent(buff, "Joel"));
}

MU_TEST(get_name)
{
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
	dfs_err err;
	size_t avail_size = 1 << 20; //1M
	char *device = "./test_create_partition.hex";

	err = dfs_pcreate(device, avail_size);
	mu_assert_int_eq(err, 0);
}

MU_TEST(open_close_partition)
{
	dfs_err err;
	dfs_partition *pt;
	size_t avail_size = 1 << 20; //1M
	char *device = "./test_open_close_partition.hex";

	err = dfs_pcreate(device, avail_size);
	mu_assert(err == 0, "Partition creation failed.");

	err = dfs_popen(device, &pt);
	mu_assert_int_eq(err, 0);

	err = dfs_pclose(pt);
	mu_assert_int_eq(err, 0);
}

MU_TEST_SUITE(dfs_partition_good)
{
	MU_RUN_TEST(create_partition);
	MU_RUN_TEST(open_close_partition);
}


//===Partition function errors===
MU_TEST(create_partition_errors)
{
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

MU_TEST_SUITE(dfs_partition_errors)
{
	MU_RUN_TEST(create_partition_errors);
	MU_RUN_TEST(open_close_partition_errors);
}


int main()
{
	MU_RUN_SUITE(dfs_path_all);
	MU_RUN_SUITE(dfs_partition_good);
	MU_RUN_SUITE(dfs_partition_errors);
	MU_REPORT();
	
	return MU_EXIT_CODE;
}


/*
//===Old tests===
void HandTest()
{
	dfs_partition *pt;
	int descriptor;
	dfs_err err;
	size_t size = (1 << 15) + 1024;
	char *data = malloc(size);
	memset(data, 0x69, size);

	if ((err = dfs_pcreate("./device.hex", 32768 * 32)))
	{ printf("Error initting partition: %d.\n", err); return; }

	if ((err = dfs_popen("./device.hex", &pt)))
	{ printf("Error opening partition: %d.\n", err); return; }

	if ((err = dfs_fcreate(pt, "Cock & Ball Torture")))
	{ printf("Error creating file: %d.\n", err); return; }

	if ((err = dfs_fopen(pt, "Cock & Ball Torture", DFS_FILEM_WRITE, &descriptor)))
	{ printf("Error opening file: %d.\n", err); return; }

	if ((err = dfs_fwrite(pt, descriptor, data, size, NULL)))
	{ printf("Error writing to file: %d.\n", err); return; }

	memset(data, 0x42, size);

	if ((err = dfs_fwrite(pt, descriptor, data, size, NULL)))
	{ printf("Error writing to file 2: %d.\n", err); return; }

	if ((err = dfs_fclose(pt, descriptor)))
	{ printf("Error closing file: %d.\n", err); return; }

	if ((err = dfs_fopen(pt, "Cock & Ball Torture", DFS_FILEM_READ, &descriptor)))
	{ printf("Error opening file 2: %d.\n", err); return; }

	if ((err = dfs_fread(pt, descriptor, data, size, NULL)))
	{ printf("Error reading from file: %d.\n", err); return; }

	for (size_t i = 0; i < size; i++)
	{
		if (data[i] != 0x69)
		{
			printf("ERR: Incorrect data, expected 0x69.\n");
			break;
		}
	}

	if ((err = dfs_fread(pt, descriptor, data, size, NULL)))
	{ printf("Error reading from file 2: %d.\n", err); return; }

	for (size_t i = 0; i < size; i++)
	{
		if (data[i] != 0x42)
		{
			printf("ERR: Incorrect data, expected 0x42.\n");
			break;
		}
	}

	if ((err = dfs_fclose(pt, descriptor)))
	{ printf("Error closing file 2: %d.\n", err); return; }

	if ((err = dfs_pclose(pt)))
	{ printf("Error closing partition: %d.\n", err); return; }

	printf("Finished successfully.\n");

	free(data);
}
*/
