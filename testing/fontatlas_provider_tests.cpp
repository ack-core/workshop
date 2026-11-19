
#include "runner.h"

namespace test {
    bool fontatlas_provider_non_uniquity_test(const API &api) {
        const resource::FontAtlasProviderPtr first = resource::FontAtlasProvider::instance(api.platform, api.rendering);
        const resource::FontAtlasProviderPtr second = resource::FontAtlasProvider::instance(api.platform, api.rendering);
        EXPECT_NOT_EQ(first, nullptr);
        EXPECT_NOT_EQ(second, nullptr);
        EXPECT_NOT_EQ(first, second);
        return true;
    }
}

namespace test {
    void registerFontAtlasProviderTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(fontatlas_provider_non_uniquity_test);    }
}
