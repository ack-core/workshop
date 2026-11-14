
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
    class resource_provider_texture_loading_test : public StepTestInterface {
    public:
        static auto make() -> std::unique_ptr<StepTestInterface> {
            return std::make_unique<resource_provider_texture_loading_test>();
        }
        void init(const API &api) override {
            const char *path = "resource_provider_tests/resource_provider_texture_loading_test";
            api.resourceProvider->getOrLoadTexture(path, [this](const foundation::RenderTexturePtr &texture) {
                _first = texture;
            });
            api.resourceProvider->getOrLoadTexture(path, [this](const foundation::RenderTexturePtr &texture) {
                _second = texture;
            });
        }
        bool testResult(const API &api) override {
            EXPECT_NOT_EQ(_first, nullptr);
            EXPECT_EQ(_first, _second);
            EXPECT_EQ(_first->getWidth(), 32);
            EXPECT_EQ(_first->getHeight(), 32);
            EXPECT_EQ(_first->getFormat(), foundation::RenderTextureFormat::RGBA8UN);
            return true;
        }

    private:
        foundation::RenderTexturePtr _first;
        foundation::RenderTexturePtr _second;
    };
}

namespace test {
    void registerResourceProviderTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(resource_provider_non_uniquity_test);
        REGISTER_STEP_TEST(resource_provider_texture_loading_test);
    }
}
