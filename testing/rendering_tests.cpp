
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
    class rendering_white_line_on_black_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<rendering_white_line_on_black_test>();
        }
        void init(const API &api) override {
            // Diagonal line in the top-left corner, 0.1 margin from screen edges in NDC
            //
            const char *shaderSrc = R"(
                fixed {
                    line[2] : float2 = [-0.9, 0.9][-0.4, 0.4]
                }
                vssrc {
                    output_position = float4(fixed_line[repeat_ID], 0.5, 1.0);
                }
                fssrc {
                    output_color[0] = float4(1.0, 1.0, 1.0, 1.0);
                }
            )";
            const foundation::InputLayout layout = foundation::InputLayout {
                .repeat = 2
            };
            _shader = api.rendering->createShader(shaderSrc, layout);
        }
        void updateAndDraw(const API &api) override {
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                rendering.applyShader(_shader, foundation::RenderTopology::LINES, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.draw(1);
            });
        }
        bool testResult(const API &api, const std::uint8_t *ref, const std::uint8_t *frame) override {
            EXPECT_NOT_EQ(_shader, nullptr);

            struct Pixel {
                std::size_t x, y;
                bool white;
            };
            const Pixel pixels[] = {
                { 30, 30, true }, { 90, 90, true }, { 150, 150, true },     // on the line
                { 20, 20, false }, { 90, 80, false }, { 160, 160, false },  // beyond ends and aside
            };

            for (const Pixel &pixel : pixels) {
                const std::size_t offset = (pixel.y * foundation::TESTING_RENDER_WIDTH + pixel.x) * 4;
                EXPECT_EQ(frame[offset + 0], pixel.white ? 255 : 0);
                EXPECT_EQ(frame[offset + 1], pixel.white ? 255 : 0);
                EXPECT_EQ(frame[offset + 2], pixel.white ? 255 : 0);
                EXPECT_EQ(frame[offset + 3], 255);
            }
            return true;
        }

    private:
        foundation::RenderShaderPtr _shader;
    };
}

namespace test {
    class rendering_red_triangle_on_black_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<rendering_red_triangle_on_black_test>();
        }
        void init(const API &api) override {
            // Triangle in the top-left corner, 0.1 margin from screen edges in NDC
            //
            const char *shaderSrc = R"(
                fixed {
                    triangle[3] : float2 = [-0.9, 0.9][-0.4, 0.9][-0.9, 0.4]
                }
                vssrc {
                    output_position = float4(fixed_triangle[repeat_ID], 0.5, 1.0);
                }
                fssrc {
                    output_color[0] = float4(1.0, 0.0, 0.0, 1.0);
                }
            )";
            const foundation::InputLayout layout = foundation::InputLayout {
                .repeat = 3
            };
            _shader = api.rendering->createShader(shaderSrc, layout);
        }
        void updateAndDraw(const API &api) override {
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                rendering.applyShader(_shader, foundation::RenderTopology::TRIANGLES, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.draw(1);
            });
        }
        bool testResult(const API &api, const std::uint8_t *ref, const std::uint8_t *frame) override {
            EXPECT_NOT_EQ(_shader, nullptr);

            struct Pixel {
                std::size_t x, y;
                bool red;
            };
            const Pixel pixels[] = {
                { 30, 30, true }, { 140, 30, true }, { 30, 140, true },     // inside corners
                { 90, 20, false }, { 20, 90, false }, { 95, 95, false },    // outside edges
            };

            for (const Pixel &pixel : pixels) {
                const std::size_t offset = (pixel.y * foundation::TESTING_RENDER_WIDTH + pixel.x) * 4;
                EXPECT_EQ(frame[offset + 0], pixel.red ? 255 : 0);
                EXPECT_EQ(frame[offset + 1], 0);
                EXPECT_EQ(frame[offset + 2], 0);
                EXPECT_EQ(frame[offset + 3], 255);
            }
            return true;
        }

    private:
        foundation::RenderShaderPtr _shader;
    };
}


namespace test {
    void registerRenderingTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(rendering_uniquty_test);
        REGISTER_SCREENSHOT_TEST(rendering_fill_green_test);
        REGISTER_SCREENSHOT_TEST(rendering_white_line_on_black_test);
        REGISTER_SCREENSHOT_TEST(rendering_red_triangle_on_black_test);
    }
}
