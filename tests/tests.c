//tests.c - Tests

#include <stdlib.h>

#include "framework/minunit.h"

#include "../src/dfs.h"
#include "../src/paths.h"

//For more thourough tests and for constants access
#include "../src/dfs_structures.h"
//#include "../src/dfs_internals.h"

#pragma region Path functions
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
#pragma endregion

#pragma region Partition functions
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
#pragma endregion

#pragma region Partition function errors
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
#pragma endregion

#pragma region Directory functions
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
#pragma endregion

#pragma region Directory function errors
MU_TEST(null_args_directories_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_directories_errors.hex";

	dfs_popen(device, &pt);

	err = dfs_dcreate(pt, NULL);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_dcreate accepted a NULL path.");
}

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

	MU_RUN_TEST(null_args_directories_errors);
	MU_RUN_TEST(duplicated_directories_errors);
	MU_RUN_TEST(empty_name_directories_errors);
}
#pragma endregion

#pragma region File functions
MU_TEST(create_file)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_files_good.hex";

	dfs_popen(device, &pt);

	err = dfs_fcreate(pt, "create.file");
	mu_assert_int_eq(0, err);

	dfs_dcreate(pt, "dir1");

	err = dfs_fcreate(pt, "dir1/create1.file");
	mu_assert_int_eq(0, err);

	dfs_pclose(pt);
}

MU_TEST(open_close_file)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_files_good.hex";

	dfs_popen(device, &pt);
	dfs_fcreate(pt, "open_close.file");
	dfs_dcreate(pt, "dir2");
	dfs_fcreate(pt, "dir2/open_close.file");

	int fd;

	err = dfs_fopen(pt, "open_close.file", DFS_FILEM_RDWR, &fd);
	mu_assert_int_eq(0, err);

	err = dfs_fclose(pt, fd);
	mu_assert_int_eq(0, err);

	err = dfs_fopen(pt, "dir2/open_close.file", DFS_FILEM_READ, &fd);
	mu_assert_int_eq(0, err);

	err = dfs_fclose(pt, fd);
	mu_assert_int_eq(0, err);

	dfs_pclose(pt);
}

MU_TEST(read_write_file)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_files_good.hex";
	char *data = "I am a test string that will be written to file.\n";

	int fd;
	size_t io;
	dfs_popen(device, &pt);
	dfs_fcreate(pt, "read_write.file");
	dfs_fopen(pt, "read_write.file", DFS_FILEM_WRITE, &fd);

	err = dfs_fwrite(pt, fd, data, strlen(data), &io);
	mu_assert_int_eq(0, err);
	mu_assert_int_eq(strlen(data), io);

	char buff[64] = { 0 };

	dfs_fclose(pt, fd);
	dfs_fopen(pt, "read_write.file", DFS_FILEM_READ, &fd);

	err = dfs_fread(pt, fd, buff, strlen(data), &io);
	mu_assert_int_eq(0, err);
	mu_assert_int_eq(strlen(data), io);
	mu_assert_string_eq(data, buff);

	dfs_fclose(pt, fd);
	dfs_pclose(pt);
}

MU_TEST(seek_file)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_files_good.hex";
	char *data = "I am a test string that will use space to be able to seek.\n";

	int fd;
	size_t pos;
	dfs_popen(device, &pt);
	dfs_fcreate(pt, "seek.file");
	dfs_fopen(pt, "seek.file", DFS_FILEM_WRITE, &fd);
	dfs_fwrite(pt, fd, data, strlen(data), NULL);

	err = dfs_fget_pos(pt, fd, &pos);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(strlen(data), pos);

	err = dfs_fset_pos(pt, fd, 0);
	mu_assert_int_eq(DFS_SUCCESS, err);

	err = dfs_fget_pos(pt, fd, &pos);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(0, pos);

	err = dfs_fset_pos(pt, fd, 5);
	mu_assert_int_eq(DFS_SUCCESS, err);

	err = dfs_fget_pos(pt, fd, &pos);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(5, pos);

	err = dfs_fset_pos(pt, fd, 500);
	mu_assert_int_eq(DFS_SUCCESS, err);

	err = dfs_fget_pos(pt, fd, &pos);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(500, pos);

	err = dfs_fset_pos(pt, fd, 0x8100);
	mu_assert_int_eq(DFS_SUCCESS, err);

	err = dfs_fget_pos(pt, fd, &pos);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(0x8100, pos);

	dfs_fclose(pt, fd);
	dfs_pclose(pt);
}

MU_TEST_SUITE(dfs_files_good)
{
	size_t avail_size = 1 << 20; //1M
	char *device = "./test_files_good.hex";

	dfs_pcreate(device, avail_size);

	MU_RUN_TEST(create_file);
	MU_RUN_TEST(open_close_file);
	MU_RUN_TEST(read_write_file);
	MU_RUN_TEST(seek_file);
}
#pragma endregion

#pragma region File function errors
MU_TEST(null_args_files_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_files_errors.hex";
	int fd;

	dfs_popen(device, &pt);

	//==fcreate==
	err = dfs_fcreate(NULL, "test.file");
	mu_assert(err == DFS_NVAL_ARGS, "dfs_fcreate accepted a NULL partition.");

	err = dfs_fcreate(pt, NULL);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_fcreate accepted a NULL path.");

	//==fopen==
	err = dfs_fopen(NULL, "test.file", DFS_FILEM_RDWR, &fd);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_fopen accepted a NULL partition.");

	err = dfs_fopen(pt, NULL, DFS_FILEM_RDWR, &fd);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_fopen accepted a NULL path.");

	err = dfs_fopen(pt, "test.file", 0, &fd);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_fopen accepted empty file mode flags.");

	err = dfs_fopen(pt, "test.file", DFS_FILEM_RDWR, NULL);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_fopen accepted a NULL fd pointer.");

	//setup
	char buff[16];
	size_t pos;
	dfs_fcreate(pt, "argtester.file");
	dfs_fopen(pt, "argtester.file", DFS_FILEM_RDWR, &fd);

	//==fclose==
	err = dfs_fclose(NULL, fd);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_fclose accepted a NULL partition.");

	//==fwrite==
	err = dfs_fwrite(NULL, fd, buff, 0, NULL);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_fwrite accepted a NULL partition.");

	err = dfs_fwrite(pt, fd, NULL, 0, NULL);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_fwrite accepted a NULL buffer.");

	//==fread==
	err = dfs_fread(NULL, fd, buff, 0, NULL);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_fread accepted a NULL partition.");

	err = dfs_fread(pt, fd, NULL, 0, NULL);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_fread accepted a NULL buffer.");

	//==fset_pos==
	err = dfs_fset_pos(NULL, fd, 0);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_fset_pos accepted a NULL partition.");

	//==fget_pos==
	err = dfs_fget_pos(NULL, fd, &pos);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_fget_pos accepted a NULL partition.");

	err = dfs_fget_pos(pt, fd, NULL);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_fget_pos accepted a NULL position.");
}

MU_TEST(duplicated_files_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_files_errors.hex";

	dfs_popen(device, &pt);
	dfs_fcreate(pt, "dup.file");

	err = dfs_fcreate(pt, "dup.file");
	mu_assert(err == DFS_ALREADY_EXISTS, "dfs_fcreate created a duplciated file.");
}

MU_TEST(empty_name_files_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_files_errors.hex";

	dfs_popen(device, &pt);

	err = dfs_fcreate(pt, "");
	mu_assert(err == DFS_NVAL_PATH, "dfs_fcreate accepted an emtpy file name.");

	int fd;
	err = dfs_fopen(pt, "", DFS_FILEM_RDWR, &fd);
	mu_assert(err == DFS_NVAL_PATH, "dfs_fopen accepted an emtpy file name.");
}

MU_TEST(invalid_dir_files_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_files_errors.hex";

	dfs_popen(device, &pt);

	err = dfs_fcreate(pt, "nodir/test.file");
	mu_assert(err == DFS_PATH_NOT_FOUND, "dfs_fcreate accepted a path to a non-existing directory.");

	int fd;
	err = dfs_fopen(pt, "nodir/test.file", DFS_FILEM_SHARE_RDWR, &fd);
	mu_assert(err == DFS_PATH_NOT_FOUND, "dfs_fopen accepted a path to a non-existing directory.");
}

//TODO: Finish tests

MU_TEST_SUITE(dfs_files_errors)
{
	size_t avail_size = 1 << 20; //1M
	char *device = "./test_files_errors.hex";

	dfs_pcreate(device, avail_size);

	MU_RUN_TEST(null_args_files_errors);
	MU_RUN_TEST(duplicated_files_errors);
	MU_RUN_TEST(empty_name_files_errors);
	MU_RUN_TEST(invalid_dir_files_errors);
}
#pragma endregion


int main()
{
	MU_RUN_SUITE(dfs_path_all);
	MU_RUN_SUITE(dfs_partition_good);
	MU_RUN_SUITE(dfs_partition_errors);
	MU_RUN_SUITE(dfs_directories_good);
	MU_RUN_SUITE(dfs_directories_errors);
	MU_RUN_SUITE(dfs_files_good);
	MU_RUN_SUITE(dfs_files_errors);
	MU_REPORT();
	
	return MU_EXIT_CODE;
}
