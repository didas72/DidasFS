//DPaths.c - Tests for DPath functions

#include "framework/minunit.h"
#include "../src/DPaths.h"

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

int main()
{
	MU_RUN_SUITE(dpath_all);
	MU_REPORT();
	
	return MU_EXIT_CODE;
}
