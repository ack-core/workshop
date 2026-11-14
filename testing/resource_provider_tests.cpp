
#include "resource_provider_tests.h"

namespace test {
    bool resource_provider_non_uniquity_test(const API &api) {
        const resource::ResourceProviderPtr first = resource::ResourceProvider::instance(api.platform, api.rendering);
        const resource::ResourceProviderPtr second = resource::ResourceProvider::instance(api.platform, api.rendering);
        EXPECT_NOT_EQ(first, nullptr);
        EXPECT_NOT_EQ(second, nullptr);
        EXPECT_NOT_EQ(first, second);
        return true;
    }
}

namespace test {
    void registerResourceProviderTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(resource_provider_non_uniquity_test);
    }
}
