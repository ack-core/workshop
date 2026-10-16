
#include "rendering_tests.h"

namespace test {
    bool rendering_uniquty_test(const API &api) {
        EXPECT_EQ(api.rendering.get(), foundation::RenderingInterface::instance(api.platform).get());
        return true;
    }
}

namespace test {
    class rendering_fill_green_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<rendering_fill_green_test>();
        }
        void init(const API &api) override {}
        void updateAndDraw(const API &api) override {
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 1.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {

            });
        }
        bool testResult(const API &api, const std::uint8_t *ref, const std::uint8_t *frame) override {
            const std::size_t topLeft = 0;
            const std::size_t middle = ((foundation::TESTING_RENDER_HEIGHT / 2) * foundation::TESTING_RENDER_WIDTH + foundation::TESTING_RENDER_WIDTH / 2) * 4;

            for (std::size_t offset : { topLeft, middle }) {
                EXPECT_EQ(frame[offset + 0], 0);
                EXPECT_EQ(frame[offset + 1], 255);
                EXPECT_EQ(frame[offset + 2], 0);
                EXPECT_EQ(frame[offset + 3], 255);
            }
            return true;
        }
    };
}


namespace test {
    void registerRenderingTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(rendering_uniquty_test);
        REGISTER_SCREENSHOT_TEST(rendering_fill_green_test);
    }
}
