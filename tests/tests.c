//DPaths.c - Tests for DPath functions

#include <stdlib.h>

#include "framework/minunit.h"

#include "../src/DidasFS.h"
#include "../src/DPaths.h"

//===Test framework stuff===

MU_TEST(combine)
{
	char buff[32]; buff[31] = '\0';

	mu_assert_string_eq("./Joel/Leal", DPathCombine(buff, "./Joel", "Leal"));
	mu_assert_string_eq("./Joel/Leal", DPathCombine(buff, "./Joel/", "Leal"));
	mu_assert_string_eq("./Joel/Leal", DPathCombine(buff, "./Joel", "/Leal"));
	mu_assert_string_eq("./Joel/Leal", DPathCombine(buff, "./Joel/", "/Leal"));
}

MU_TEST(get_parent)
{
	char buff[32]; buff[31] = '\0';

	mu_assert_string_eq("./Joel/asmr", DPathGetParent(buff, "./Joel/asmr/bdsm/"));
	mu_assert_string_eq("./Joel", DPathGetParent(buff, "./Joel/asmr"));
	mu_assert_string_eq("Joel", DPathGetParent(buff, "Joel/Joel"));
	mu_assert_string_eq("", DPathGetParent(buff, "Joel"));
}

MU_TEST(get_name)
{
	char buff[32]; buff[31] = '\0';

	mu_assert_string_eq("bdsm", DPathGetName(buff, "./Joel/asmr/bdsm/"));
	mu_assert_string_eq("asmr", DPathGetName(buff, "./Joel/asmr"));
	mu_assert_string_eq("Joel", DPathGetName(buff, "Joel/Joel"));
	mu_assert_string_eq("Joel", DPathGetName(buff, "Joel"));
}

MU_TEST_SUITE(dpath_all)
{
	MU_RUN_TEST(combine);
	MU_RUN_TEST(get_parent);
	MU_RUN_TEST(get_name);
}



//===Hand testing stuff===
void HandTest()
{
	DPartition *pt;
	DFileStream *fs;
	int err;
	size_t size = (1 << 15) + 1024;
	char *data = malloc(size);
	memset(data, 0x69, size);

	if ((err = InitPartition("./device.hex", 32768 * 32)))
	{ printf("Error initting partition: %d.\n", err); return; }

	if ((err = OpenPartition("./device.hex", &pt)))
	{ printf("Error opening partition: %d.\n", err); return; }

	if ((err = CreateFile(pt, "Cock & Ball Torture")))
	{ printf("Error creating file: %d.\n", err); return; }

	if ((err = OpenFile(pt, "Cock & Ball Torture", &fs)))
	{ printf("Error opening file: %d.\n", err); return; }

	if ((err = FileWrite(data, size, fs, NULL)))
	{ printf("Error writing to file: %d.\n", err); return; }

	memset(data, 0x42, size);

	if ((err = FileWrite(data, size, fs, NULL)))
	{ printf("Error writing to file 2: %d.\n", err); return; }

	if ((err = CloseFile(fs)))
	{ printf("Error closing file: %d.\n", err); return; }

	if ((err = OpenFile(pt, "Cock & Ball Torture", &fs)))
	{ printf("Error opening file 2: %d.\n", err); return; }

	if ((err = FileRead(data, size, fs, NULL)))
	{ printf("Error reading from file: %d.\n", err); return; }

	for (size_t i = 0; i < size; i++)
	{
		if (data[i] != 0x69)
		{
			printf("ERR: Incorrect data, expected 0x69.\n");
			break;
		}
	}

	if ((err = FileRead(data, size, fs, NULL)))
	{ printf("Error reading from file 2: %d.\n", err); return; }

	for (size_t i = 0; i < size; i++)
	{
		if (data[i] != 0x42)
		{
			printf("ERR: Incorrect data, expected 0x42.\n");
			break;
		}
	}

	if ((err = CloseFile(fs)))
	{ printf("Error closing file 2: %d.\n", err); return; }

	if ((err = ClosePartition(pt)))
	{ printf("Error closing partition: %d.\n", err); return; }

	printf("Finished successfully.\n");

	free(data);
}



int main()
{
	//Test framework
	MU_RUN_SUITE(dpath_all);
	MU_REPORT();

	printf("Hand tests:\n");
	HandTest();
	
	return MU_EXIT_CODE;
}
