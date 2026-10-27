
#include "util_tests.h"

namespace {
    int g_tmp_call_counter = 0;
}

namespace test {
    bool util_callback_test(const API &api) {
        util::callback<void()> cb0 = []() {
            g_tmp_call_counter++;
        };
        cb0();
        EXPECT_EQ(g_tmp_call_counter, 1);
        return true;
    }
}


namespace test {
    void registerUtilTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(util_callback_test);
    }
}
