#include <stdio.h>
#include <string.h>

#include "framework/unity.h"
#include "framework/unity_fixture.h"

#include "mocks.h"
#include "mock_utils.h"
#include "mocks_interface.h"

#include "../src/dfs.h"
#include "../src/dfs_structures.h"


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



// =======================
// ===== ERROR TESTS =====
// =======================



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
