
#include "math_tests.h"

namespace test {
    bool math_vector2f_test(API&& api) {
        return true;
    }
}


namespace test {
    void registerMathTests(TestRunner& runner) {
        runner.registerUnitTest("math_vector2f_test", math_vector2f_test);
    }
}
