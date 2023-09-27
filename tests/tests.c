//DPaths.c - Tests for DPath functions

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
	int err;

	if ((err = InitPartition("./device.hex", 32768 * 32)))
	{ printf("Error initting partition: %d.\n", err); return; }

	if ((err = OpenPartition("./device.hex", &pt)))
	{ printf("Error opening partition: %d.\n", err); return; }

	if ((err = CreateFile(pt, "Cock & Ball Torture")))
	{ printf("Error creating file: %d.\n", err); return; }

	printf("Finished successfully.\n");
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
