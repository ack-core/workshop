
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
    class resource_provider_cube_texture_loading_test : public StepTestInterface {
    public:
        static auto make() -> std::unique_ptr<StepTestInterface> {
            return std::make_unique<resource_provider_cube_texture_loading_test>();
        }
        void init(const API &api) override {
            api.resourceProvider->getOrLoadCubeTexture(PATH, [this, provider = api.resourceProvider](const foundation::RenderTexturePtr &first) {
                _first = first;
                provider->getOrLoadCubeTexture(PATH, [this, provider](const foundation::RenderTexturePtr &second) {
                    _second = second;
                    provider->getOrLoadCubeTexture("resource_provider_tests/resource_provider_texture_loading_test", [this, provider](const foundation::RenderTexturePtr &invalid) {
                        _invalidLoaded = true;
                        _invalid = invalid;
                        provider->removeCubeTexture(PATH);
                        provider->getOrLoadCubeTexture(PATH, [this](const foundation::RenderTexturePtr &reloaded) {
                            _reloaded = reloaded;
                        });
                    });
                });
            });
        }
        bool testResult(const API &api) override {
            EXPECT_NOT_EQ(_first, nullptr);
            EXPECT_EQ(_first, _second);
            EXPECT_EQ(_first->getWidth(), 4);
            EXPECT_EQ(_first->getHeight(), 4);
            EXPECT_EQ(_first->getFormat(), foundation::RenderTextureFormat::RGBA8UN);
            EXPECT_TRUE(_invalidLoaded);
            EXPECT_EQ(_invalid, nullptr);
            EXPECT_NOT_EQ(_reloaded, nullptr);
            EXPECT_NOT_EQ(_reloaded, _first);
            return true;
        }

    private:
        static constexpr const char *PATH = "resource_provider_tests/resource_provider_cube_texture_loading_test";

        foundation::RenderTexturePtr _first;
        foundation::RenderTexturePtr _second;
        foundation::RenderTexturePtr _invalid;
        foundation::RenderTexturePtr _reloaded;
        bool _invalidLoaded = false;
    };
}

namespace test {
    class resource_provider_description_loading_test: public StepTestInterface {
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
    class resource_provider_voxel_mesh_loading_test : public StepTestInterface {
    public:
        static auto make() -> std::unique_ptr<StepTestInterface> {
            return std::make_unique<resource_provider_voxel_mesh_loading_test>();
        }
        void init(const API &api) override {
            api.resourceProvider->getOrLoadVoxelMesh(PATH, [this, provider = api.resourceProvider](const std::vector<foundation::RenderDataPtr> &first, const util::Description &desc) {
                _first = first;
                _config = desc.getInteger("v", 0);
                _info = provider->getMeshInfo(PATH);
                provider->getOrLoadVoxelMesh(PATH, [this, provider](const std::vector<foundation::RenderDataPtr> &second, const util::Description &) {
                    _second = second;
                    provider->getOrLoadVoxelMesh("resource_provider_tests/missing", [this, provider](const std::vector<foundation::RenderDataPtr> &missing, const util::Description &) {
                        _missingLoaded = true;
                        _missingEmpty = missing.empty();
                        provider->removeMesh(PATH);
                        provider->getOrLoadVoxelMesh(PATH, [this](const std::vector<foundation::RenderDataPtr> &reloaded, const util::Description &) {
                            _reloaded = reloaded;
                        });
                    });
                });
            });
        }
        bool testResult(const API &api) override {
            EXPECT_EQ(_first.size(), 2);
            EXPECT_EQ(_first[0]->getVertexCount(), 1);
            EXPECT_EQ(_first[1]->getVertexCount(), 2);
            EXPECT_EQ(_config, 3);
            EXPECT_NOT_EQ(_info, nullptr);
            EXPECT_EQ(_info->sizeX, 2);
            EXPECT_EQ(_info->sizeY, 3);
            EXPECT_EQ(_info->sizeZ, 4);
            EXPECT_EQ(_first, _second);
            EXPECT_TRUE(_missingLoaded);
            EXPECT_TRUE(_missingEmpty);
            EXPECT_EQ(_reloaded.size(), 2);
            EXPECT_NOT_EQ(_reloaded[0], _first[0]);
            return true;
        }

    private:
        static constexpr const char *PATH = "resource_provider_tests/resource_provider_voxel_mesh_loading_test";

        std::vector<foundation::RenderDataPtr> _first;
        std::vector<foundation::RenderDataPtr> _second;
        std::vector<foundation::RenderDataPtr> _reloaded;
        const resource::MeshInfo *_info = nullptr;
        std::int64_t _config = 0;
        bool _missingLoaded = false;
        bool _missingEmpty = false;
    };
}

namespace test {
    class resource_provider_voxel_mesh_scale_test : public StepTestInterface {
    public:
        static auto make() -> std::unique_ptr<StepTestInterface> {
            return std::make_unique<resource_provider_voxel_mesh_scale_test>();
        }
        void init(const API &api) override {
            api.resourceProvider->getOrLoadVoxelMesh("resource_provider_tests/resource_provider_voxel_mesh_scale_test", [this](const std::vector<foundation::RenderDataPtr> &frames, const util::Description &) {
                _frames = frames;
            });
        }
        bool testResult(const API &api) override {
            EXPECT_EQ(_frames.size(), 2);
            EXPECT_EQ(_frames[0]->getVertexCount(), 1);
            EXPECT_EQ(_frames[1]->getVertexCount(), 1);
            return true;
        }

    private:
        std::vector<foundation::RenderDataPtr> _frames;
    };
}

namespace test {
    bool resource_provider_prefabs_test(const API &api) {
        const resource::ResourceProviderPtr provider = resource::ResourceProvider::instance(api.platform, api.rendering);
        std::vector<std::uint8_t> data = {'P', 'R', 'E', 'F', 'A', 'B', 'S', '!', 2, 0, 0, 0};

        auto putString = [&data](const std::string &str) {
            const std::uint32_t size = std::uint32_t(str.length() + 5);
            data.insert(data.end(), reinterpret_cast<const std::uint8_t *>(&size), reinterpret_cast<const std::uint8_t *>(&size) + 4);
            data.insert(data.end(), str.begin(), str.end());
            data.push_back(0);
        };

        putString("prefabs/first");
        putString("i : integer = 7");
        putString("prefabs/second");
        putString("s : string = \"abc\"");

        EXPECT_TRUE(provider->setPrefabsData(data.data(), data.size()));
        EXPECT_NOT_EQ(provider->getPrefab("prefabs/first"), nullptr);
        EXPECT_EQ(provider->getPrefab("prefabs/first")->getInteger("i", 0), 7);
        EXPECT_NOT_EQ(provider->getPrefab("prefabs/second"), nullptr);
        EXPECT_EQ(provider->getPrefab("prefabs/second")->getString("s", ""), "abc");
        EXPECT_EQ(provider->getPrefab("prefabs/unknown"), nullptr);

        EXPECT_FALSE(provider->setPrefabsData(data.data(), 11));
        EXPECT_FALSE(provider->setPrefabsData(nullptr, data.size()));
        data[0] = 'X';
        EXPECT_FALSE(provider->setPrefabsData(data.data(), data.size()));
        EXPECT_NOT_EQ(provider->getPrefab("prefabs/first"), nullptr);
        return true;
    }
}

namespace test {
    void registerResourceProviderTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(resource_provider_non_uniquity_test);
        REGISTER_UNIT_TEST(resource_provider_prefabs_test);
        REGISTER_STEP_TEST(resource_provider_texture_loading_test);
        REGISTER_STEP_TEST(resource_provider_cube_texture_loading_test);
        REGISTER_STEP_TEST(resource_provider_description_loading_test);
        REGISTER_STEP_TEST(resource_provider_description_reload_test);
        REGISTER_STEP_TEST(resource_provider_voxel_mesh_loading_test);
        REGISTER_STEP_TEST(resource_provider_voxel_mesh_scale_test);
    }
}
