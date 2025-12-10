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

#pragma GCC diagnostic ignored "-Wformat-truncation"


#pragma region Path functions
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
#pragma endregion

#pragma region Partition functions
TEST_GROUP(partition_good);

TEST_SETUP(partition_good)
{
	mock_setup();
}

TEST_TEAR_DOWN(partition_good) { }

TEST(partition_good, create_partition)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	size_t avail_size = 1 << 20; //1M
	char *device = "./test_create_partition.hex";

	err = dfs_pcreate(device, avail_size);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);

	//TODO: Max size
}

TEST(partition_good, open_close_partition)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	size_t avail_size = 1 << 20; //1M
	char *device = "./test_open_close_partition.hex";

	err = dfs_pcreate(device, avail_size);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_SUCCESS, err, "Partition creation failed.");

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
#pragma endregion

#pragma region Partition function errors
TEST_GROUP(partition_err);

TEST_SETUP(partition_err)
{
	mock_setup();
}

TEST_TEAR_DOWN(partition_err) { }

TEST(partition_err, create_partition_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
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

	dfs_err err;
	dfs_partition *pt;
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
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_CORRUPTED_PARTITION, err, "dfs_popen accepted a corrupted partition.");
}

TEST_GROUP_RUNNER(partition_err)
{
	RUN_TEST_CASE(partition_err, create_partition_errors);
	RUN_TEST_CASE(partition_err, open_close_partition_errors);
	RUN_TEST_CASE(partition_err, open_corrupt_partition_errors);
}
#pragma endregion

#pragma region Directory functions
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
#pragma endregion

#pragma region Directory function errors
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
#pragma endregion

#pragma region File functions
TEST_GROUP(file_good);

TEST_SETUP(file_good)
{
	mock_setup();

	size_t avail_size = 1 << 20; //1M
	char *device = "./test_files_good.hex";

	dfs_pcreate(device, avail_size);
}

TEST_TEAR_DOWN(file_good) { }

TEST(file_good, create_file)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_files_good.hex";

	dfs_popen(device, &pt);

	err = dfs_fcreate(pt, "create.file");
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);

	dfs_dcreate(pt, "dir1");

	err = dfs_fcreate(pt, "dir1/create1.file");
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);

	dfs_pclose(pt);
}

TEST(file_good, open_close_file)
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
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);

	err = dfs_fclose(pt, fd);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);

	err = dfs_fopen(pt, "dir2/open_close.file", DFS_FILEM_READ, &fd);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);

	err = dfs_fclose(pt, fd);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);

	dfs_pclose(pt);
}

TEST(file_good, read_write_file)
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
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(strlen(data), io);

	char buff[64] = { 0 };

	dfs_fclose(pt, fd);
	dfs_fopen(pt, "read_write.file", DFS_FILEM_READ, &fd);

	err = dfs_fread(pt, fd, buff, strlen(data), &io);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(strlen(data), io);
	TEST_ASSERT_EQUAL_STRING(data, buff);

	dfs_fclose(pt, fd);
	dfs_fcreate(pt, "long_file.file");
	dfs_fopen(pt, "long_file.file", DFS_FILEM_WRITE, &fd);

	char long_data[BLOCK_DATA_SIZE + 16];
	memset(long_data, 0x5A, BLOCK_DATA_SIZE + 16);
	long_data[BLOCK_DATA_SIZE + 15] = 0;

	err = dfs_fwrite(pt, fd, long_data, BLOCK_DATA_SIZE + 16, &io);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(BLOCK_DATA_SIZE + 16, io);

	dfs_fclose(pt, fd);
	dfs_fopen(pt, "long_file.file", DFS_FILEM_READ, &fd);

	char long_buff[BLOCK_DATA_SIZE + 16];

	err = dfs_fread(pt, fd, long_buff, BLOCK_DATA_SIZE + 16, &io);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(BLOCK_DATA_SIZE + 16, io);
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, strncmp(long_data, long_buff, BLOCK_DATA_SIZE + 16), "fread result differs from expected used on a long file."); //REVIEW
	
	//TODO: Test RW not aligned on block start

	dfs_fclose(pt, fd);
	dfs_pclose(pt);
}

TEST(file_good, read_eof)
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
	TEST_ASSERT_EQUAL_INT(0, err);
	TEST_ASSERT_EQUAL_INT_MESSAGE(0, io, "fread read past the end of file (aligned).");

	dfs_fseek(pt, fd, len - 2, SEEK_SET);

	//unaligned with file end
	err = dfs_fread(pt, fd, buff, 1024, &io);
	TEST_ASSERT_EQUAL_INT(0, err);
	TEST_ASSERT_EQUAL_INT_MESSAGE(2, io, "fread read past the end of file (unaligned).");

	//TODO: File size matched data in block

	dfs_fclose(pt, fd);
	dfs_pclose(pt);
}

TEST(file_good, seek_file)
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
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(strlen(data), pos);

	err = dfs_fseek(pt, fd, 0, DFS_SEEK_SET);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);

	err = dfs_fget_pos(pt, fd, &pos);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(0, pos);

	err = dfs_fseek(pt, fd, 5, DFS_SEEK_SET);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);

	err = dfs_fget_pos(pt, fd, &pos);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(5, pos);

	err = dfs_fseek(pt, fd, 500, DFS_SEEK_SET);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);

	err = dfs_fget_pos(pt, fd, &pos);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(500, pos);

	err = dfs_fseek(pt, fd, 0x8100, DFS_SEEK_SET);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);

	err = dfs_fget_pos(pt, fd, &pos);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(0x8100, pos);

	//TODO: Test other values of WHENCE

	dfs_fclose(pt, fd);
	dfs_pclose(pt);
}

TEST(file_good, read_write_align_file)
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
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(BLOCK_DATA_SIZE, io);

	dfs_fclose(pt, fd);
	dfs_fopen(pt, "read_write_align.file", DFS_FILEM_RDWR, &fd);

	err = dfs_fread(pt, fd, data, BLOCK_DATA_SIZE, &io);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(BLOCK_DATA_SIZE, io);
	for (int i = 0; i < BLOCK_DATA_SIZE; i++) 
	{
		if (data[i] != -1)
		{
			TEST_FAIL_MESSAGE("Read bad data after aligned write.\n");
			break;
		}
	}

	//Check cursor set properly
	err = dfs_fget_pos(pt, fd, &io);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(BLOCK_DATA_SIZE, io);
	
	//Read with cursor aligned on block boundary and at file end
	err = dfs_fread(pt, fd, data, BLOCK_DATA_SIZE, &io);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(0, io); //If not 0, block alignment is a problem

	//Append another whole block
	err = dfs_fwrite(pt, fd, data, BLOCK_DATA_SIZE, &io);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(BLOCK_DATA_SIZE, io);

	dfs_fclose(pt, fd);
	dfs_fopen(pt, "read_write_align.file", DFS_FILEM_READ, &fd);
	err = dfs_fread(pt, fd, data, BLOCK_DATA_SIZE, &io); //Seek to end of first block

	err = dfs_fread(pt, fd, data, BLOCK_DATA_SIZE, &io);
	TEST_ASSERT_EQUAL_INT(DFS_SUCCESS, err);
	TEST_ASSERT_EQUAL_INT(BLOCK_DATA_SIZE, io); //If didn't read, block alignment is a problem

	dfs_fclose(pt, fd);
	free(data);
}

TEST_GROUP_RUNNER(file_good)
{
	RUN_TEST_CASE(file_good, create_file);
	RUN_TEST_CASE(file_good, open_close_file);
	RUN_TEST_CASE(file_good, read_write_file);
	RUN_TEST_CASE(file_good, read_eof);
	RUN_TEST_CASE(file_good, seek_file);
	RUN_TEST_CASE(file_good, read_write_align_file);
}
#pragma endregion

#pragma region File function errors
TEST_GROUP(file_err);

TEST_SETUP(file_err)
{
	mock_setup();
	
	size_t avail_size = 1 << 20; //1M
	char *device = "./test_files_errors.hex";

	dfs_pcreate(device, avail_size);
}

TEST_TEAR_DOWN(file_err) { }

TEST(file_err, null_args_files_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_files_errors.hex";
	int fd;

	dfs_popen(device, &pt);

	//==fcreate==
	err = dfs_fcreate(NULL, "test.file");
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_fcreate accepted a NULL partition.");

	err = dfs_fcreate(pt, NULL);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_fcreate accepted a NULL path.");

	//==fopen==
	err = dfs_fopen(NULL, "test.file", DFS_FILEM_RDWR, &fd);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_fopen accepted a NULL partition.");

	err = dfs_fopen(pt, NULL, DFS_FILEM_RDWR, &fd);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_fopen accepted a NULL path.");

	err = dfs_fopen(pt, "test.file", 0, &fd);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_fopen accepted empty file mode flags.");

	err = dfs_fopen(pt, "test.file", DFS_FILEM_RDWR, NULL);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_fopen accepted a NULL fd pointer.");

	//setup
	char buff[16];
	size_t pos;
	dfs_fcreate(pt, "argtester.file");
	dfs_fopen(pt, "argtester.file", DFS_FILEM_RDWR, &fd);

	//==fclose==
	err = dfs_fclose(NULL, fd);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_fclose accepted a NULL partition.");

	//==fwrite==
	err = dfs_fwrite(NULL, fd, buff, 0, NULL);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_fwrite accepted a NULL partition.");

	err = dfs_fwrite(pt, fd, NULL, 0, NULL);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_fwrite accepted a NULL buffer.");

	//==fread==
	err = dfs_fread(NULL, fd, buff, 0, NULL);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_fread accepted a NULL partition.");

	err = dfs_fread(pt, fd, NULL, 0, NULL);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_fread accepted a NULL buffer.");

	//==fseek==
	err = dfs_fseek(NULL, fd, 0, DFS_SEEK_SET);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_fset_pos accepted a NULL partition.");
	err = dfs_fseek(pt, fd, 0, 500);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_fset_pos accepted an invalid whence value.");

	//==fget_pos==
	err = dfs_fget_pos(NULL, fd, &pos);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_fget_pos accepted a NULL partition.");

	err = dfs_fget_pos(pt, fd, NULL);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_ARGS, err, "dfs_fget_pos accepted a NULL position.");
}

TEST(file_err, duplicated_files_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_files_errors.hex";

	dfs_popen(device, &pt);
	dfs_fcreate(pt, "dup.file");

	err = dfs_fcreate(pt, "dup.file");
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_ALREADY_EXISTS, err, "dfs_fcreate created a duplciated file.");
}

TEST(file_err, empty_name_files_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_files_errors.hex";

	dfs_popen(device, &pt);

	err = dfs_fcreate(pt, "");
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_PATH, err, "dfs_fcreate accepted an emtpy file name.");

	int fd;
	err = dfs_fopen(pt, "", DFS_FILEM_RDWR, &fd);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_NVAL_PATH, err, "dfs_fopen accepted an emtpy file name.");
}

TEST(file_err, invalid_dir_files_errors)
{
	fprintf(stderr, "\nEntering %s\n\n", __func__);

	dfs_err err;
	dfs_partition *pt;
	char *device = "./test_files_errors.hex";

	dfs_popen(device, &pt);

	err = dfs_fcreate(pt, "nodir/test.file");
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_PATH_NOT_FOUND, err, "dfs_fcreate accepted a path to a non-existing directory.");

	int fd;
	err = dfs_fopen(pt, "nodir/test.file", DFS_FILEM_SHARE_RDWR, &fd);
	TEST_ASSERT_EQUAL_INT_MESSAGE(DFS_PATH_NOT_FOUND, err, "dfs_fopen accepted a path to a non-existing directory.");
}

TEST_GROUP_RUNNER(file_err)
{
	RUN_TEST_CASE(file_err, null_args_files_errors);
	RUN_TEST_CASE(file_err, duplicated_files_errors);
	RUN_TEST_CASE(file_err, empty_name_files_errors);
	RUN_TEST_CASE(file_err, invalid_dir_files_errors);
}
#pragma endregion

#pragma region Management functions
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
#pragma endregion

#pragma region Management function errors
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
#pragma endregion



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
