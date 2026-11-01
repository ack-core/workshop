
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
    class rendering_static_data_triangle_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<rendering_static_data_triangle_test>();
        }
        void init(const API &api) override {
            const char *shaderSrc = R"(
                inout {
                    color : float4
                }
                vssrc {
                    output_position = float4(vertex_position, 0.5, 1.0);
                    output_color = vertex_color;
                }
                fssrc {
                    output_color[0] = input_color;
                }
            )";
            const foundation::InputLayout layout = foundation::InputLayout {
                .attributes = {
                    {"position", foundation::InputAttributeFormat::FLOAT2},
                    {"color", foundation::InputAttributeFormat::FLOAT4},
                }
            };
            // Triangle in the bottom-right corner, 0.1 margin from screen edges in NDC
            //
            const float vertexes[] = {
                0.9f, -0.9f, 1.0f, 0.0f, 0.0f, 1.0f,
                0.4f, -0.9f, 0.0f, 1.0f, 0.0f, 1.0f,
                0.9f, -0.4f, 0.0f, 0.0f, 1.0f, 1.0f,
            };
            _shader = api.rendering->createShader(shaderSrc, layout);
            _data = api.rendering->createData(layout, vertexes, 3);
        }
        void updateAndDraw(const API &api) override {
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                rendering.applyShader(_shader, foundation::RenderTopology::TRIANGLES, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.draw(_data);
            });
        }
        bool testResult(const API &api, const std::uint8_t *ref, const std::uint8_t *frame) override {
            EXPECT_NOT_EQ(_shader, nullptr);
            EXPECT_NOT_EQ(_data, nullptr);

            struct Pixel {
                std::size_t x, y;
                int r, g, b;
            };
            const Pixel pixels[] = {
                { 484, 484, 255, 0, 0 }, { 361, 484, 0, 255, 0 }, { 484, 361, 0, 0, 255 },  // near corners
                { 420, 491, 0, 0, 0 }, { 491, 420, 0, 0, 0 }, { 417, 417, 0, 0, 0 },        // outside edges
            };
            const int tolerance = 16;

            for (const Pixel &pixel : pixels) {
                const std::size_t offset = (pixel.y * foundation::TESTING_RENDER_WIDTH + pixel.x) * 4;
                EXPECT_LT(std::abs(frame[offset + 0] - pixel.r), tolerance);
                EXPECT_LT(std::abs(frame[offset + 1] - pixel.g), tolerance);
                EXPECT_LT(std::abs(frame[offset + 2] - pixel.b), tolerance);
                EXPECT_EQ(frame[offset + 3], 255);
            }
            return true;
        }

    private:
        foundation::RenderShaderPtr _shader;
        foundation::RenderDataPtr _data;
    };
}

namespace test {
    class rendering_strip_quads_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<rendering_strip_quads_test>();
        }
        void init(const API &api) override {
            // Two strip quads: red in the top-left, green in the bottom-right, 0.1 margin from screen edges and center in NDC
            //
            const char *shaderSrc = R"(
                fixed {
                    quad[4] : float2 = [0.0, 0.8][0.8, 0.8][0.0, 0.0][0.8, 0.0]
                    offset[2] : float2 = [-0.9, 0.1][0.1, -0.9]
                    color[2] : float4 = [1.0, 0.0, 0.0, 1.0][0.0, 1.0, 0.0, 1.0]
                }
                inout {
                    color : float4
                }
                vssrc {
                    output_position = float4(fixed_quad[repeat_ID] + fixed_offset[vertex_ID], 0.5, 1.0);
                    output_color = fixed_color[vertex_ID];
                }
                fssrc {
                    output_color[0] = input_color;
                }
            )";
            const foundation::InputLayout layout = foundation::InputLayout {
                .repeat = 4
            };
            _shader = api.rendering->createShader(shaderSrc, layout);
        }
        void updateAndDraw(const API &api) override {
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                rendering.applyShader(_shader, foundation::RenderTopology::TRIANGLESTRIP, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.draw(2);
            });
        }
        bool testResult(const API &api, const std::uint8_t *ref, const std::uint8_t *frame) override {
            EXPECT_NOT_EQ(_shader, nullptr);

            auto pixelIs = [frame](std::size_t x, std::size_t y, int r, int g) {
                const std::uint8_t *p = frame + (y * foundation::TESTING_RENDER_WIDTH + x) * 4;
                return p[0] == r && p[1] == g && p[2] == 0 && p[3] == 255;
            };

            EXPECT_TRUE(pixelIs(35, 35, 255, 0));    // red quad: top-left (1st triangle)
            EXPECT_TRUE(pixelIs(220, 35, 255, 0));   // red quad: top-right
            EXPECT_TRUE(pixelIs(35, 220, 255, 0));   // red quad: bottom-left
            EXPECT_TRUE(pixelIs(220, 220, 255, 0));  // red quad: bottom-right (2nd triangle)
            EXPECT_TRUE(pixelIs(128, 128, 255, 0));  // red quad: center
            EXPECT_TRUE(pixelIs(291, 291, 0, 255));  // green quad: top-left (1st triangle)
            EXPECT_TRUE(pixelIs(476, 291, 0, 255));  // green quad: top-right
            EXPECT_TRUE(pixelIs(291, 476, 0, 255));  // green quad: bottom-left
            EXPECT_TRUE(pixelIs(476, 476, 0, 255));  // green quad: bottom-right (2nd triangle)
            EXPECT_TRUE(pixelIs(384, 384, 0, 255));  // green quad: center
            EXPECT_TRUE(pixelIs(15, 128, 0, 0));     // outside red quad: left
            EXPECT_TRUE(pixelIs(240, 128, 0, 0));    // outside red quad: right
            EXPECT_TRUE(pixelIs(128, 240, 0, 0));    // outside red quad: bottom
            EXPECT_TRUE(pixelIs(496, 384, 0, 0));    // outside green quad: right
            EXPECT_TRUE(pixelIs(384, 496, 0, 0));    // outside green quad: bottom
            EXPECT_TRUE(pixelIs(384, 128, 0, 0));    // empty top-right corner
            EXPECT_TRUE(pixelIs(128, 384, 0, 0));    // empty bottom-left corner
            return true;
        }

    private:
        foundation::RenderShaderPtr _shader;
    };
}

namespace test {
    class rendering_shader_constants_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<rendering_shader_constants_test>();
        }
        void init(const API &api) override {
            // Triangle in the top-left corner, moved and colored by constants
            //
            const char *shaderSrc = R"(
                fixed {
                    triangle[3] : float2 = [-0.9, 0.9][-0.4, 0.9][-0.9, 0.4]
                }
                const {
                    offset : float4
                    color : float4
                }
                vssrc {
                    output_position = float4(fixed_triangle[repeat_ID], 0.5, 1.0) + const_offset;
                }
                fssrc {
                    output_color[0] = const_color;
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

                // Constants have to be applied per draw: the second draw must not reuse values of the first one
                //
                const Constants left = { { 0.0f, 0.0f, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } };
                const Constants right = { { 1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } };

                rendering.applyShaderConstants(&left);
                rendering.draw(1);
                rendering.applyShaderConstants(&right);
                rendering.draw(1);
            });
        }
        bool testResult(const API &api, const std::uint8_t *ref, const std::uint8_t *frame) override {
            EXPECT_NOT_EQ(_shader, nullptr);

            // One check per line: the line in the log tells which draw failed
            //
            EXPECT_TRUE(isPixel(frame, 40, 40, 255, 0));    // first draw, red triangle
            EXPECT_TRUE(isPixel(frame, 300, 40, 0, 255));   // second draw, blue triangle
            EXPECT_TRUE(isPixel(frame, 220, 40, 0, 0));     // between triangles
            EXPECT_TRUE(isPixel(frame, 40, 300, 0, 0));     // below first triangle
            EXPECT_TRUE(isPixel(frame, 300, 300, 0, 0));    // below second triangle
            return true;
        }

    private:
        struct Constants {
            math::vector4f offset;
            math::vector4f color;
        };

        static bool isPixel(const std::uint8_t *frame, std::size_t x, std::size_t y, std::uint8_t r, std::uint8_t b) {
            const std::uint8_t *pixel = frame + (y * foundation::TESTING_RENDER_WIDTH + x) * 4;
            return pixel[0] == r && pixel[1] == 0 && pixel[2] == b && pixel[3] == 255;
        }

        foundation::RenderShaderPtr _shader;
    };
}


namespace test {
    void registerRenderingTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(rendering_uniquty_test);
        REGISTER_SCREENSHOT_TEST(rendering_fill_green_test);
        REGISTER_SCREENSHOT_TEST(rendering_white_line_on_black_test);
        REGISTER_SCREENSHOT_TEST(rendering_red_triangle_on_black_test);
        REGISTER_SCREENSHOT_TEST(rendering_static_data_triangle_test);
        REGISTER_SCREENSHOT_TEST(rendering_strip_quads_test);
        REGISTER_SCREENSHOT_TEST(rendering_shader_constants_test);
    }
}
