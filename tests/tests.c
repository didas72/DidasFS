//tests.c - Tests

#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "framework/minunit.h"
#include "mocks.h"
#include "mock_utils.h"
#include "mocks_interface.h"

#include "../src/dfs.h"
#include "../src/paths.h"

//For more thourough tests and for constants access
#include "../src/dfs_structures.h"
//#include "../src/dfs_internals.h"

#pragma GCC diagnostic ignored "-Wformat-truncation"


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
void partition_good_test_setup()
{
	mock_setup();
}

void partition_good_test_teardown()
{

}

MU_TEST(create_partition)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	size_t avail_size = 1 << 20; //1M
	char *device = "./test_create_partition.hex";

	err = dfs_pcreate(device, avail_size);
	mu_assert_int_eq(DFS_SUCCESS, err);

	//TODO: Max size
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
	mu_assert_int_eq(DFS_SUCCESS, err);

	err = dfs_pclose(pt);
	mu_assert_int_eq(DFS_SUCCESS, err);
}

MU_TEST_SUITE(dfs_partition_good)
{
	MU_SUITE_CONFIGURE(&partition_good_test_setup, &partition_good_test_teardown);

	MU_RUN_TEST(create_partition);
	MU_RUN_TEST(open_close_partition);
}
#pragma endregion

#pragma region Partition function errors
void partition_err_test_setup()
{
	mock_setup();
}

void partition_err_test_teardown()
{

}

MU_TEST(create_partition_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	size_t avail_size = 1 << 20; //1M
	char *device = "./test_create_partition_erros.hex";

	err = dfs_pcreate(NULL, avail_size);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_pcreate accepted a NULL device.");

	err = dfs_pcreate(device, 0);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_pcreate accepted 0 as available size.");

	err = dfs_pcreate(device, BLOCK_SIZE + SECTOR_SIZE - 1);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_pcreate accepted a value too small for available size.");

	err = dfs_pcreate(device, BLOCK_SIZE + SECTOR_SIZE);
	mu_assert(err == DFS_SUCCESS, "dfs_pcreate rejected the minimum value for available size.");

	//TODO: Larger than max size
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
	int fd = open(device, O_CREAT | O_TRUNC | O_WRONLY, S_IRUSR | S_IWUSR);
	lseek(fd, 0x100000 - 1, SEEK_SET);
	write(fd, &zero, 1);
	close(fd);

	err = dfs_popen(device, &pt);
	mu_assert(err == DFS_CORRUPTED_PARTITION, "dfs_popen accepted a corrupted partition.");
}

MU_TEST_SUITE(dfs_partition_errors)
{
	MU_SUITE_CONFIGURE(&partition_err_test_setup, &partition_err_test_teardown);

	MU_RUN_TEST(create_partition_errors);
	MU_RUN_TEST(open_close_partition_errors);
	MU_RUN_TEST(open_corrupt_partition_errors);
}
#pragma endregion

#pragma region Directory functions
void directory_good_test_setup()
{
	mock_setup();

	size_t avail_size = 1 << 20; //1M
	char *device = "./test_directories_good.hex";

	dfs_pcreate(device, avail_size);
}

void directory_good_test_teardown()
{

}

MU_TEST(create_directory)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_directories_good.hex";

	dfs_popen(device, &pt);

	err = dfs_dcreate(pt, "test dir");
	mu_assert_int_eq(DFS_SUCCESS, err);

	err = dfs_dcreate(pt, "im at root");
	mu_assert_int_eq(DFS_SUCCESS, err);

	dfs_pclose(pt);
}

MU_TEST(create_directory_nested)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_directories_good.hex";

	dfs_popen(device, &pt);
	dfs_dcreate(pt, "test dir");

	err = dfs_dcreate(pt, "test dir/more test dirs");
	mu_assert_int_eq(DFS_SUCCESS, err);

	err = dfs_dcreate(pt, "test dir/testing...");
	mu_assert_int_eq(DFS_SUCCESS, err);

	err = dfs_dcreate(pt, "test dir/more inside");
	mu_assert_int_eq(DFS_SUCCESS, err);

	err = dfs_dcreate(pt, "test dir/more inside/im here");
	mu_assert_int_eq(DFS_SUCCESS, err);

	dfs_pclose(pt);
}

MU_TEST_SUITE(dfs_directories_good)
{
	MU_SUITE_CONFIGURE(&directory_good_test_setup, &directory_good_test_teardown);
	
	MU_RUN_TEST(create_directory);
	MU_RUN_TEST(create_directory_nested);
}
#pragma endregion

#pragma region Directory function errors
void directory_err_test_setup()
{
	mock_setup();

	size_t avail_size = 1 << 20; //1M
	char *device = "./test_directories_errors.hex";

	dfs_pcreate(device, avail_size);
}

void directory_err_test_teardown()
{

}

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

MU_TEST(object_inside_files_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_directories_errors.hex";

	dfs_popen(device, &pt);
	dfs_fcreate(pt, "file_not_dir");

	err = dfs_dcreate(pt, "file_not_dir/file");
	mu_assert(err == DFS_NVAL_PATH, "dfs_dcreate allowed the creation of directory under a file.");
}

MU_TEST_SUITE(dfs_directories_errors)
{
	MU_SUITE_CONFIGURE(&directory_err_test_setup, &directory_err_test_teardown);

	MU_RUN_TEST(null_args_directories_errors);
	MU_RUN_TEST(duplicated_directories_errors);
	MU_RUN_TEST(empty_name_directories_errors);
	MU_RUN_TEST(object_inside_files_errors);
}
#pragma endregion

#pragma region File functions
void file_good_test_setup()
{
	mock_setup();

	size_t avail_size = 1 << 20; //1M
	char *device = "./test_files_good.hex";

	dfs_pcreate(device, avail_size);
}

void file_good_test_teardown()
{

}

MU_TEST(create_file)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_files_good.hex";

	dfs_popen(device, &pt);

	err = dfs_fcreate(pt, "create.file");
	mu_assert_int_eq(DFS_SUCCESS, err);

	dfs_dcreate(pt, "dir1");

	err = dfs_fcreate(pt, "dir1/create1.file");
	mu_assert_int_eq(DFS_SUCCESS, err);

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
	mu_assert_int_eq(DFS_SUCCESS, err);

	err = dfs_fclose(pt, fd);
	mu_assert_int_eq(DFS_SUCCESS, err);

	err = dfs_fopen(pt, "dir2/open_close.file", DFS_FILEM_READ, &fd);
	mu_assert_int_eq(DFS_SUCCESS, err);

	err = dfs_fclose(pt, fd);
	mu_assert_int_eq(DFS_SUCCESS, err);

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
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(strlen(data), io);

	char buff[64] = { 0 };

	dfs_fclose(pt, fd);
	dfs_fopen(pt, "read_write.file", DFS_FILEM_READ, &fd);

	err = dfs_fread(pt, fd, buff, strlen(data), &io);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(strlen(data), io);
	mu_assert_string_eq(data, buff);

	dfs_fclose(pt, fd);
	dfs_fcreate(pt, "long_file.file");
	dfs_fopen(pt, "long_file.file", DFS_FILEM_WRITE, &fd);

	char long_data[BLOCK_DATA_SIZE + 16];
	memset(long_data, 0x5A, BLOCK_DATA_SIZE + 16);
	long_data[BLOCK_DATA_SIZE + 15] = 0;

	err = dfs_fwrite(pt, fd, long_data, BLOCK_DATA_SIZE + 16, &io);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(BLOCK_DATA_SIZE + 16, io);

	dfs_fclose(pt, fd);
	dfs_fopen(pt, "long_file.file", DFS_FILEM_READ, &fd);

	char long_buff[BLOCK_DATA_SIZE + 16];

	err = dfs_fread(pt, fd, long_buff, BLOCK_DATA_SIZE + 16, &io);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(BLOCK_DATA_SIZE + 16, io);
	mu_assert(strncmp(long_data, long_buff, BLOCK_DATA_SIZE + 16) == 0, "fread result differs from expected used on a long file.");
	
	//TODO: Test RW not aligned on block start

	dfs_fclose(pt, fd);
	dfs_pclose(pt);
}

MU_TEST(read_eof)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_files_good.hex";
	char data[] = "Test string";
	char *buff = malloc(1024);

	int fd;
	size_t io;
	size_t len = strlen(data);
	dfs_popen(device, &pt);
	dfs_fcreate(pt, "short.file");
	dfs_fopen(pt, "short.file", DFS_FILEM_WRITE, &fd);
	dfs_fwrite(pt, fd, data, len, NULL);
	dfs_fclose(pt, fd);
	dfs_fopen(pt, "short.file", DFS_FILEM_READ, &fd);
	dfs_fread(pt, fd, buff, len, NULL);

	//aligned with file end
	err = dfs_fread(pt, fd, buff, 1024, &io);
	mu_assert_int_eq(0, err);
	mu_assert(io == 0, "fread read past the end of file (aligned).");

	dfs_fseek(pt, fd, len - 2, SEEK_SET);

	//unaligned with file end
	err = dfs_fread(pt, fd, buff, 1024, &io);
	mu_assert_int_eq(0, err);
	mu_assert(io == 2, "fread read past the end of file (unaligned).");

	//TODO: File size matched data in block

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

	err = dfs_fseek(pt, fd, 0, DFS_SEEK_SET);
	mu_assert_int_eq(DFS_SUCCESS, err);

	err = dfs_fget_pos(pt, fd, &pos);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(0, pos);

	err = dfs_fseek(pt, fd, 5, DFS_SEEK_SET);
	mu_assert_int_eq(DFS_SUCCESS, err);

	err = dfs_fget_pos(pt, fd, &pos);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(5, pos);

	err = dfs_fseek(pt, fd, 500, DFS_SEEK_SET);
	mu_assert_int_eq(DFS_SUCCESS, err);

	err = dfs_fget_pos(pt, fd, &pos);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(500, pos);

	err = dfs_fseek(pt, fd, 0x8100, DFS_SEEK_SET);
	mu_assert_int_eq(DFS_SUCCESS, err);

	err = dfs_fget_pos(pt, fd, &pos);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(0x8100, pos);

	//TODO: Test other values of WHENCE

	dfs_fclose(pt, fd);
	dfs_pclose(pt);
}

MU_TEST(read_write_align_file)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_files_good.hex";
	char *data = malloc(BLOCK_DATA_SIZE);
	memset(data, 0xFF, BLOCK_DATA_SIZE);

	int fd;
	size_t io;
	dfs_popen(device, &pt);
	dfs_fcreate(pt, "read_write_align.file");
	dfs_fopen(pt, "read_write_align.file", DFS_FILEM_WRITE, &fd);

	//Write one block
	err = dfs_fwrite(pt, fd, data, BLOCK_DATA_SIZE, &io);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(BLOCK_DATA_SIZE, io);

	dfs_fclose(pt, fd);
	dfs_fopen(pt, "read_write_align.file", DFS_FILEM_RDWR, &fd);

	err = dfs_fread(pt, fd, data, BLOCK_DATA_SIZE, &io);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(BLOCK_DATA_SIZE, io);
	for (int i = 0; i < BLOCK_DATA_SIZE; i++) 
	{
		if (data[i] != -1)
		{
			mu_fail("Read bad data after aligned write.\n");
			break;
		}
	}

	//Check cursor set properly
	err = dfs_fget_pos(pt, fd, &io);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(BLOCK_DATA_SIZE, io);
	
	//Read with cursor aligned on block boundary and at file end
	err = dfs_fread(pt, fd, data, BLOCK_DATA_SIZE, &io);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(0, io); //If not 0, block alignment is a problem

	//Append another whole block
	err = dfs_fwrite(pt, fd, data, BLOCK_DATA_SIZE, &io);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(BLOCK_DATA_SIZE, io);

	dfs_fclose(pt, fd);
	dfs_fopen(pt, "read_write_align.file", DFS_FILEM_READ, &fd);
	err = dfs_fread(pt, fd, data, BLOCK_DATA_SIZE, &io); //Seek to end of first block

	err = dfs_fread(pt, fd, data, BLOCK_DATA_SIZE, &io);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(BLOCK_DATA_SIZE, io); //If didn't read, block alignment is a problem

	dfs_fclose(pt, fd);
	free(data);
}

MU_TEST_SUITE(dfs_files_good)
{
	MU_SUITE_CONFIGURE(&file_good_test_setup, &file_good_test_teardown);

	MU_RUN_TEST(create_file);
	MU_RUN_TEST(open_close_file);
	MU_RUN_TEST(read_write_file);
	MU_RUN_TEST(seek_file);
	MU_RUN_TEST(read_write_align_file);
	MU_RUN_TEST(read_eof);
}
#pragma endregion

#pragma region File function errors
void file_err_test_setup()
{
	mock_setup();
	
	size_t avail_size = 1 << 20; //1M
	char *device = "./test_files_errors.hex";

	dfs_pcreate(device, avail_size);
}

void file_err_test_teardown()
{

}

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

	//==fseek==
	err = dfs_fseek(NULL, fd, 0, DFS_SEEK_SET);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_fset_pos accepted a NULL partition.");
	err = dfs_fseek(pt, fd, 0, 500);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_fset_pos accepted an invalid whence value.");

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

MU_TEST_SUITE(dfs_files_errors)
{
	MU_SUITE_CONFIGURE(&file_err_test_setup, &file_err_test_teardown);

	MU_RUN_TEST(null_args_files_errors);
	MU_RUN_TEST(duplicated_files_errors);
	MU_RUN_TEST(empty_name_files_errors);
	MU_RUN_TEST(invalid_dir_files_errors);
}
#pragma endregion

#pragma region Management functions
void management_good_test_setup()
{
	mock_setup();
	
	size_t avail_size = 1 << 20; //1M
	char *device = "./test_management_good.hex";

	dfs_pcreate(device, avail_size);
}

void management_good_test_teardown()
{

}

MU_TEST(list_entries)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_management_good.hex";

	size_t count;
	dfs_entry entries[16] = { 0 };
	dfs_popen(device, &pt);
	
	err = dfs_dlist_entries(pt, "", 0, NULL, &count);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(0, count);

	err = dfs_dlist_entries(pt, "", 16, entries, &count);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(0, count);

	dfs_fcreate(pt, "file1.test");
	dfs_dcreate(pt, "dir1");

	err = dfs_dlist_entries(pt, "", 0, NULL, &count);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(2, count);

	err = dfs_dlist_entries(pt, "", 16, entries, &count);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(2, count);
	//FIXME: Ordering is not required
	mu_assert_int_eq(false, entries[0].dir);
	mu_assert_int_eq(0, entries[0].length);
	mu_assert_string_eq("file1.test", entries[0].name);
	mu_assert_int_eq(true, entries[1].dir);
	mu_assert_int_eq(0, entries[1].length);
	mu_assert_string_eq("dir1", entries[1].name);

	err = dfs_dlist_entries(pt, "dir1", 0, NULL, &count);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(0, count);

	err = dfs_dlist_entries(pt, "dir1", 16, entries, &count);
	mu_assert_int_eq(DFS_SUCCESS, err);
	mu_assert_int_eq(0, count);

	dfs_pclose(pt);
}

MU_TEST_SUITE(dfs_management_good)
{
	MU_SUITE_CONFIGURE(&management_good_test_setup, &management_good_test_teardown);

	MU_RUN_TEST(list_entries);
}
#pragma endregion

#pragma region Management function errors
void management_err_test_setup()
{
	mock_setup();
	
	size_t avail_size = 1 << 20; //1M
	char *device = "./test_management_errors.hex";

	dfs_pcreate(device, avail_size);
}

void management_err_test_teardown()
{

}

MU_TEST(list_entries_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_management_errors.hex";

	size_t count;
	dfs_entry entries[16] = { 0 };
	dfs_popen(device, &pt);

	err = dfs_dlist_entries(NULL, "", 16, entries, &count);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_list_entries accepted a NULL partition.");

	err = dfs_dlist_entries(pt, NULL, 16, entries, &count);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_dlist_entries accepted a NULL path.");

	err = dfs_dlist_entries(pt, "", 16, NULL, &count);
	mu_assert(err == DFS_NVAL_ARGS, "dfs_dlist_entries accepted a NULL entries pointer with a non zero capacity.");

	dfs_pclose(pt);
}

MU_TEST_SUITE(dfs_management_errors)
{
	MU_SUITE_CONFIGURE(&management_err_test_setup, &management_err_test_teardown);

	MU_RUN_TEST(list_entries_errors);
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
	MU_RUN_SUITE(dfs_management_good);
	MU_RUN_SUITE(dfs_management_errors);
	MU_REPORT();

	return MU_EXIT_CODE;
}
