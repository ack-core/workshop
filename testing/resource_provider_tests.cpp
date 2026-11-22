
#include "runner.h"

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
    class resource_provider_description_loading_test : public StepTestInterface {
    public:
        static auto make() -> std::unique_ptr<StepTestInterface> {
            return std::make_unique<resource_provider_description_loading_test>();
        }
        void init(const API &api) override {
            const char *path = "resource_provider_tests/resource_provider_description_loading_test";
            api.resourceProvider->getOrLoadDescription(path, [this, provider = api.resourceProvider, path](const util::Description &first) {
                _first = &first;
                provider->getOrLoadDescription(path, [this, provider](const util::Description &second) {
                    _second = &second;
                    provider->getOrLoadDescription("resource_provider_tests/missing", [this](const util::Description &missing) {
                        _missingLoaded = true;
                        _missingEmpty = missing.empty();
                    });
                });
            });
        }
        bool testResult(const API &api) override {
            EXPECT_NOT_EQ(_first, nullptr);
            EXPECT_EQ(_first, _second);
            EXPECT_EQ(_first->size(), 3);
            EXPECT_EQ(_first->getInteger("i", 0), 7);
            EXPECT_EQ(_first->getString("s", ""), "abc");
            EXPECT_NOT_EQ(_first->getDescription("d"), nullptr);
            EXPECT_TRUE(_first->getDescription("d")->getBool("b", false));
            EXPECT_TRUE(_missingLoaded);
            EXPECT_TRUE(_missingEmpty);
            return true;
        }

    private:
        const util::Description *_first = nullptr;
        const util::Description *_second = nullptr;
        bool _missingLoaded = false;
        bool _missingEmpty = false;
    };
}

namespace test {
    class resource_provider_description_reload_test : public StepTestInterface {
    public:
        static auto make() -> std::unique_ptr<StepTestInterface> {
            return std::make_unique<resource_provider_description_reload_test>();
        }
        void init(const API &api) override {
            _save(api, "v : integer = 1", [this, api] {
                api.resourceProvider->getOrLoadDescription(PATH, [this, api](const util::Description &) {
                    _save(api, "v : integer = 2", [this, api] {
                        api.resourceProvider->getOrLoadDescription(PATH, [this, api](const util::Description &cached) {
                            _cachedValue = cached.getInteger("v", 0);
                            api.resourceProvider->removeDescription(PATH);
                            api.resourceProvider->getOrLoadDescription(PATH, [this, api](const util::Description &reloaded) {
                                _reloadedValue = reloaded.getInteger("v", 0);
                                api.platform->saveFile(FILE, nullptr, 0, [](bool) {}, true);
                            });
                        });
                    });
                });
            });
        }
        bool testResult(const API &api) override {
            EXPECT_EQ(_cachedValue, 1);
            EXPECT_EQ(_reloadedValue, 2);
            return true;
        }

    private:
        static constexpr const char *PATH = "resource_provider_tests/description_saved";
        static constexpr const char *FILE = "resource_provider_tests/description_saved.txt";

        static void _save(const API &api, const char *content, util::callback<void()> &&next) {
            api.platform->saveFile(FILE, reinterpret_cast<const std::uint8_t *>(content), std::strlen(content), [next = std::move(next)](bool) mutable {
                next();
            }, true);
        }

        std::int64_t _cachedValue = 0;
        std::int64_t _reloadedValue = 0;
    };
}

namespace test {
    void registerResourceProviderTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(resource_provider_non_uniquity_test);
        REGISTER_STEP_TEST(resource_provider_texture_loading_test);
        REGISTER_STEP_TEST(resource_provider_description_loading_test);
        REGISTER_STEP_TEST(resource_provider_description_reload_test);
    }
}
