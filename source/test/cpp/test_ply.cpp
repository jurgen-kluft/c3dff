#include "cbase/c_target.h"
#include "cunittest/cunittest.h"

using namespace ncore;

extern unsigned char   skull_txt[];
extern unsigned int    skull_txt_len;

UNITTEST_SUITE_BEGIN(ply)
{
    UNITTEST_FIXTURE(main)
    {
        UNITTEST_FIXTURE_SETUP() {}
        UNITTEST_FIXTURE_TEARDOWN() {}

        UNITTEST_TEST(test_read)
        {

        }
    }
}
UNITTEST_SUITE_END
