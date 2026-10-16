
#include "platform_tests.h"

namespace test {
    bool platform_uniquty_test(const API &api) {
        EXPECT_EQ(api.platform.get(), foundation::PlatformInterface::instance().get());
        return true;
    }
}

namespace test {
    class platform_file_loading_test : public StepTestInterface {
    public:
        static auto make() -> std::unique_ptr<StepTestInterface> {
            return std::make_unique<platform_file_loading_test>();
        }
        void init(const API &api) override {
            api.platform->loadFile("platform_tests/file_5.txt", [this](ByteDataPtr &&data, std::size_t size) {
                if (data) {
                    _content.assign(reinterpret_cast<const char *>(data.get()), size);
                }
            });
        }
        bool testResult(const API &api) override {
            EXPECT_EQ(_content.size(), 5);
            EXPECT_EQ(_content, "test5");
            return true;
        }

    private:
        std::string _content;
    };
}

namespace test {
    class platform_file_save_and_loading_chain_test : public StepTestInterface {
    public:
        static auto make() -> std::unique_ptr<StepTestInterface> {
            return std::make_unique<platform_file_save_and_loading_chain_test>();
        }
        void init(const API &api) override {
            const char *content = "write-read";
            api.platform->saveFile("platform_tests/file_saved.txt", reinterpret_cast<const std::uint8_t *>(content), 10, [this, platform = api.platform](bool result) {
                _saved = result;
                platform->loadFile("platform_tests/file_saved.txt", [this](ByteDataPtr &&data, std::size_t size) {
                    if (data) {
                        _content.assign(reinterpret_cast<const char *>(data.get()), size);
                    }
                });
            });
        }
        bool testResult(const API &api) override {
            EXPECT_TRUE(_saved);
            EXPECT_EQ(_content.size(), 10);
            EXPECT_EQ(_content, "write-read");
            return true;
        }

    private:
        bool _saved = false;
        std::string _content;
    };
}

namespace test {
    class platform_file_delete_and_loading_fail_test : public StepTestInterface {
    public:
        static auto make() -> std::unique_ptr<StepTestInterface> {
            return std::make_unique<platform_file_delete_and_loading_fail_test>();
        }
        void init(const API &api) override {
            api.platform->saveFile("platform_tests/file_saved.txt", nullptr, 0, [this, platform = api.platform](bool result) {
                _deleted = result;
                platform->loadFile("platform_tests/file_saved.txt", [this](ByteDataPtr &&data, std::size_t size) {
                    _loaded = true;
                    _failed = data == nullptr && size == 0;
                });
            });
        }
        bool testResult(const API &api) override {
            EXPECT_TRUE(_deleted);
            EXPECT_TRUE(_loaded);
            EXPECT_TRUE(_failed);
            return true;
        }

    private:
        bool _deleted = false;
        bool _loaded = false;
        bool _failed = false;
    };
}


namespace test {
    void registerPlatformTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(platform_uniquty_test);
        REGISTER_STEP_TEST(platform_file_loading_test);
        REGISTER_STEP_TEST(platform_file_save_and_loading_chain_test);
        REGISTER_STEP_TEST(platform_file_delete_and_loading_fail_test);
    }
}
