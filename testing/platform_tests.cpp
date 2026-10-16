
#include "math_tests.h"

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
        auto getType() const -> StepType override {
            return StepType::ASYNC_WORK_COMPLETE;
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
    void registerPlatformTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(platform_uniquty_test);
        REGISTER_STEP_TEST(platform_file_loading_test);
    }
}
