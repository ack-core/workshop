
#include "math_tests.h"

namespace test {
    bool math_vector2f_test(const API &api) {
        EXPECT_EQ(10, 10);
        return true;
    }
}


namespace test {
    void registerMathTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(math_vector2f_test);
    }
}
