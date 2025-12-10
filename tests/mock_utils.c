#include "mocks.h"

void mock_init()
{
#ifdef MOCK_DEVICE
	ram_reset_files(0);
#endif
}

void mock_setup()
{
#ifdef MOCK_DEVICE
	ram_reset_files(1);
#endif
}
