
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

    bool fontatlas_provider_not_loaded_test(const API &api) {
        const resource::FontAtlasProviderPtr provider = resource::FontAtlasProvider::instance(api.platform, api.rendering);
        const math::vector2f width = provider->getTextWidth("text", 16);
        EXPECT_EQ_F(width.x, 0.0f);
        EXPECT_EQ_F(width.y, 0.0f);

        std::vector<resource::FontCharInfo> info;
        EXPECT_EQ(provider->getTextFontAtlas("text", 16, 0, info), nullptr);
        return true;
    }
}

namespace test {
    void registerFontAtlasProviderTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(fontatlas_provider_non_uniquity_test);
        REGISTER_UNIT_TEST(fontatlas_provider_not_loaded_test);
    }
}
