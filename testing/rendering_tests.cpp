
#include "rendering_tests.h"

namespace test {
    bool rendering_uniquty_test(const API &api) {
        EXPECT_EQ(api.rendering.get(), foundation::RenderingInterface::instance(api.platform).get());
        return true;
    }
}

namespace test {

}


namespace test {
    void registerRenderingTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(rendering_uniquty_test);
    }
}
