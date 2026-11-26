
#include "runner.h"

namespace test {
    bool rendering_uniquty_test(const API &api) {
        EXPECT_EQ(api.rendering.get(), foundation::RenderingInterface::instance(api.platform).get());
        return true;
    }

    bool rendering_back_buffer_size_test(const API &api) {
        const math::vector2f size = api.rendering->getBackBufferSize();
        EXPECT_EQ(size.x, api.platform->getScreenWidth());
        EXPECT_EQ(size.y, api.platform->getScreenHeight());
        return true;
    }

    bool rendering_statistics_test(const API &api) {
        const foundation::RenderingInterface::StatInfo before = api.rendering->getStatistics();
        const char *shaderSrc = R"(
            vssrc {
                output_position = float4(vertex_position, 0.5, 1.0);
            }
            fssrc {
                output_color[0] = float4(1.0, 1.0, 1.0, 1.0);
            }
        )";
        const foundation::InputLayout layout = foundation::InputLayout {
            .attributes = {
                {"position", foundation::InputAttributeFormat::FLOAT2},
            }
        };
        const float vertexes[] = { 0.0f, 0.0f };
        const std::uint32_t pixel = 0xffffffff;
        {
            foundation::RenderShaderPtr shader = api.rendering->createShader(shaderSrc, layout);
            foundation::RenderShaderPtr sameShader = api.rendering->createShader(shaderSrc, layout);
            foundation::RenderTexturePtr texture = api.rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, 1, 1, { &pixel });
            foundation::RenderTargetPtr target = api.rendering->createRenderTarget(foundation::RenderTextureFormat::RGBA8UN, 1, 4, 4, true);
            foundation::RenderDataPtr data = api.rendering->createData(layout, vertexes, 1);

            const foundation::RenderingInterface::StatInfo &stats = api.rendering->getStatistics();
            EXPECT_NOT_EQ(shader, nullptr);
            EXPECT_EQ(shader, sameShader);
            EXPECT_EQ(stats.shadersCount, before.shadersCount + 1);
            EXPECT_EQ(stats.texturesCount, before.texturesCount + 3);
            EXPECT_EQ(stats.targetsCount, before.targetsCount + 1);
            EXPECT_EQ(stats.dataCount, before.dataCount + 1);
            EXPECT_EQ(stats.objectsCount, before.objectsCount + 6);
        }
        const foundation::RenderingInterface::StatInfo &after = api.rendering->getStatistics();
        EXPECT_EQ(after.shadersCount, before.shadersCount);
        EXPECT_EQ(after.texturesCount, before.texturesCount);
        EXPECT_EQ(after.targetsCount, before.targetsCount);
        EXPECT_EQ(after.dataCount, before.dataCount);
        EXPECT_EQ(after.objectsCount, before.objectsCount);
        return true;
    }
}

namespace test {
    class rendering_fill_green_test : public PixelTestInterface {
    public:
        static auto make() -> std::unique_ptr<PixelTestInterface> {
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
    class rendering_white_line_on_black_test : public PixelTestInterface {
    public:
        static auto make() -> std::unique_ptr<PixelTestInterface> {
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
    class rendering_red_triangle_on_black_test : public PixelTestInterface {
    public:
        static auto make() -> std::unique_ptr<PixelTestInterface> {
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
    class rendering_static_data_triangle_test : public PixelTestInterface {
    public:
        static auto make() -> std::unique_ptr<PixelTestInterface> {
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
    class rendering_strip_quads_test : public PixelTestInterface {
    public:
        static auto make() -> std::unique_ptr<PixelTestInterface> {
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
    class rendering_shader_constants_test : public PixelTestInterface {
    public:
        static auto make() -> std::unique_ptr<PixelTestInterface> {
            return std::make_unique<rendering_shader_constants_test>();
        }
        void init(const API &api) override {
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
            _frameId = api.rendering->getStatistics().frameId;
        }
        void updateAndDraw(const API &api) override {
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                rendering.applyShader(_shader, foundation::RenderTopology::TRIANGLES, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);

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

            EXPECT_TRUE(isPixel(frame, 40, 40, 255, 0));    // first draw, red triangle
            EXPECT_TRUE(isPixel(frame, 300, 40, 0, 255));   // second draw, blue triangle
            EXPECT_TRUE(isPixel(frame, 220, 40, 0, 0));     // between triangles
            EXPECT_TRUE(isPixel(frame, 40, 300, 0, 0));     // below first triangle
            EXPECT_TRUE(isPixel(frame, 300, 300, 0, 0));    // below second triangle
            EXPECT_LT(_frameId, api.rendering->getStatistics().frameId);
            EXPECT_LT(5, api.rendering->getStatistics().lastFrameVertices);
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
        std::uint64_t _frameId = 0;
    };
}

namespace test {
    class rendering_textured_quad_test : public PixelTestInterface {
    public:
        static auto make() -> std::unique_ptr<PixelTestInterface> {
            return std::make_unique<rendering_textured_quad_test>();
        }
        void init(const API &api) override {
            // Fullscreen quad sampling 2x2 texture: red, green in the top row, blue, white in the bottom row
            //
            const char *shaderSrc = R"(
                fixed {
                    quad[4] : float2 = [-1.0, 1.0][1.0, 1.0][-1.0, -1.0][1.0, -1.0]
                    uv[4] : float2 = [0.0, 0.0][1.0, 0.0][0.0, 1.0][1.0, 1.0]
                }
                inout {
                    uv : float2
                }
                vssrc {
                    output_position = float4(fixed_quad[repeat_ID], 0.5, 1.0);
                    output_uv = fixed_uv[repeat_ID];
                }
                fssrc {
                    output_color[0] = _tex2d(0, input_uv);
                }
            )";
            const foundation::InputLayout layout = foundation::InputLayout {
                .repeat = 4
            };
            const std::uint32_t pixels[] = { 0xff0000ff, 0xff00ff00, 0xffff0000, 0xffffffff };
            _shader = api.rendering->createShader(shaderSrc, layout);
            _texture = api.rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, 2, 2, { pixels });
        }
        void updateAndDraw(const API &api) override {
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                rendering.applyShader(_shader, foundation::RenderTopology::TRIANGLESTRIP, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.applyTextures({ { _texture, foundation::SamplerType::NEAREST } });
                rendering.draw(1);
            });
        }
        bool testResult(const API &api, const std::uint8_t *ref, const std::uint8_t *frame) override {
            EXPECT_NOT_EQ(_shader, nullptr);
            EXPECT_NOT_EQ(_texture, nullptr);
            EXPECT_EQ_F(_texture->getUVStart().x, 0.0f);
            EXPECT_EQ_F(_texture->getUVStart().y, 0.0f);
            EXPECT_EQ_F(_texture->getUVSize().x, 1.0f);
            EXPECT_EQ_F(_texture->getUVSize().y, 1.0f);

            auto pixelIs = [frame](std::size_t x, std::size_t y, int r, int g, int b) {
                const std::uint8_t *p = frame + (y * foundation::TESTING_RENDER_WIDTH + x) * 4;
                return p[0] == r && p[1] == g && p[2] == b && p[3] == 255;
            };

            const std::size_t w = foundation::TESTING_RENDER_WIDTH;
            const std::size_t h = foundation::TESTING_RENDER_HEIGHT;

            EXPECT_TRUE(pixelIs(w / 4, h / 4, 255, 0, 0));              // top-left texel: red
            EXPECT_TRUE(pixelIs(w * 3 / 4, h / 4, 0, 255, 0));          // top-right texel: green
            EXPECT_TRUE(pixelIs(w / 4, h * 3 / 4, 0, 0, 255));          // bottom-left texel: blue
            EXPECT_TRUE(pixelIs(w * 3 / 4, h * 3 / 4, 255, 255, 255));  // bottom-right texel: white
            return true;
        }

    private:
        foundation::RenderShaderPtr _shader;
        foundation::RenderTexturePtr _texture;
    };
}

namespace test {
    class rendering_render_target_test : public PixelTestInterface {
    public:
        static auto make() -> std::unique_ptr<PixelTestInterface> {
            return std::make_unique<rendering_render_target_test>();
        }
        void init(const API &api) override {
            // Red triangle in the top-left quadrant of the blue target, then target texture drawn as fullscreen quad
            //
            const char *triangleSrc = R"(
                fixed {
                    triangle[3] : float2 = [-1.0, 1.0][0.0, 1.0][-1.0, 0.0]
                }
                vssrc {
                    output_position = float4(fixed_triangle[repeat_ID], 0.5, 1.0);
                }
                fssrc {
                    output_color[0] = float4(1.0, 0.0, 0.0, 1.0);
                }
            )";
            const char *quadSrc = R"(
                fixed {
                    quad[4] : float2 = [-1.0, 1.0][1.0, 1.0][-1.0, -1.0][1.0, -1.0]
                    uv[4] : float2 = [0.0, 0.0][1.0, 0.0][0.0, 1.0][1.0, 1.0]
                }
                inout {
                    uv : float2
                }
                vssrc {
                    output_position = float4(fixed_quad[repeat_ID], 0.5, 1.0);
                    output_uv = fixed_uv[repeat_ID];
                }
                fssrc {
                    output_color[0] = _tex2d(0, input_uv);
                }
            )";
            _triangleShader = api.rendering->createShader(triangleSrc, foundation::InputLayout { .repeat = 3 });
            _quadShader = api.rendering->createShader(quadSrc, foundation::InputLayout { .repeat = 4 });
            _target = api.rendering->createRenderTarget(foundation::RenderTextureFormat::RGBA8UN, 1, 64, 64, false);
        }
        void updateAndDraw(const API &api) override {
            api.rendering->forTarget(_target, nullptr, math::color{ 0.0f, 0.0f, 1.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                rendering.applyShader(_triangleShader, foundation::RenderTopology::TRIANGLES, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.draw(1);
            });
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                rendering.applyShader(_quadShader, foundation::RenderTopology::TRIANGLESTRIP, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.applyTextures({ { _target->getTexture(0), foundation::SamplerType::NEAREST } });
                rendering.draw(1);
            });
        }
        bool testResult(const API &api, const std::uint8_t *ref, const std::uint8_t *frame) override {
            EXPECT_NOT_EQ(_triangleShader, nullptr);
            EXPECT_NOT_EQ(_quadShader, nullptr);
            EXPECT_NOT_EQ(_target, nullptr);
            EXPECT_EQ(_target->getWidth(), 64);
            EXPECT_EQ(_target->getHeight(), 64);
            EXPECT_EQ(_target->getTextureCount(), 1);
            EXPECT_EQ_F(_target->getTexture(0)->getUVStart().x, 0.0f);
            EXPECT_EQ_F(_target->getTexture(0)->getUVStart().y, 0.0f);
            EXPECT_EQ_F(_target->getTexture(0)->getUVSize().x, 1.0f);
            EXPECT_EQ_F(_target->getTexture(0)->getUVSize().y, 1.0f);

            auto pixelIs = [frame](std::size_t x, std::size_t y, int r, int b) {
                const std::uint8_t *p = frame + (y * foundation::TESTING_RENDER_WIDTH + x) * 4;
                return p[0] == r && p[1] == 0 && p[2] == b && p[3] == 255;
            };

            const std::size_t w = foundation::TESTING_RENDER_WIDTH;
            const std::size_t h = foundation::TESTING_RENDER_HEIGHT;

            EXPECT_TRUE(pixelIs(w / 8, h / 8, 255, 0));           // triangle: top-left
            EXPECT_TRUE(pixelIs(w * 3 / 8, h / 16, 255, 0));      // triangle: top-right corner
            EXPECT_TRUE(pixelIs(w / 16, h * 3 / 8, 255, 0));      // triangle: bottom-left corner
            EXPECT_TRUE(pixelIs(w * 3 / 8, h * 3 / 8, 0, 255));   // cleared: outside triangle hypotenuse
            EXPECT_TRUE(pixelIs(w * 7 / 8, h / 8, 0, 255));       // cleared: top-right
            EXPECT_TRUE(pixelIs(w / 8, h * 7 / 8, 0, 255));       // cleared: bottom-left
            EXPECT_TRUE(pixelIs(w * 7 / 8, h * 7 / 8, 0, 255));   // cleared: bottom-right
            return true;
        }

    private:
        foundation::RenderShaderPtr _triangleShader;
        foundation::RenderShaderPtr _quadShader;
        foundation::RenderTargetPtr _target;
    };
}

namespace test {
    class rendering_instanced_data_test : public PixelTestInterface {
    public:
        static auto make() -> std::unique_ptr<PixelTestInterface> {
            return std::make_unique<rendering_instanced_data_test>();
        }
        void init(const API &api) override {
            // Three instances shifted right by instance_ID with red, green, blue colors:
            // top row - triangle from data, bottom rows - repeated quads at two data origins
            //
            const char *triangleSrc = R"(
                fixed {
                    color[3] : float4 = [1.0, 0.0, 0.0, 1.0][0.0, 1.0, 0.0, 1.0][0.0, 0.0, 1.0, 1.0]
                }
                inout {
                    color : float4
                }
                vssrc {
                    output_position = float4(vertex_position + float2(0.6 * float(instance_ID), 0.0), 0.5, 1.0);
                    output_color = fixed_color[instance_ID];
                }
                fssrc {
                    output_color[0] = input_color;
                }
            )";
            const char *quadSrc = R"(
                fixed {
                    quad[4] : float2 = [0.0, 0.0][0.4, 0.0][0.0, -0.3][0.4, -0.3]
                    color[3] : float4 = [1.0, 0.0, 0.0, 1.0][0.0, 1.0, 0.0, 1.0][0.0, 0.0, 1.0, 1.0]
                }
                inout {
                    color : float4
                }
                vssrc {
                    output_position = float4(vertex_position + fixed_quad[repeat_ID] + float2(0.6 * float(instance_ID), 0.0), 0.5, 1.0);
                    output_color = fixed_color[instance_ID];
                }
                fssrc {
                    output_color[0] = input_color;
                }
            )";
            const foundation::InputLayout triangleLayout = foundation::InputLayout {
                .attributes = {
                    {"position", foundation::InputAttributeFormat::FLOAT2},
                }
            };
            const foundation::InputLayout quadLayout = foundation::InputLayout {
                .repeat = 4,
                .attributes = {
                    {"position", foundation::InputAttributeFormat::FLOAT2},
                }
            };
            const float triangle[] = { -0.9f, 0.9f, -0.5f, 0.9f, -0.9f, 0.5f };
            const float origins[] = { -0.9f, -0.1f, -0.9f, -0.6f };
            _triangleShader = api.rendering->createShader(triangleSrc, triangleLayout);
            _quadShader = api.rendering->createShader(quadSrc, quadLayout);
            _triangleData = api.rendering->createData(triangleLayout, triangle, 3);
            _quadData = api.rendering->createData(quadLayout, origins, 2);
        }
        void updateAndDraw(const API &api) override {
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                rendering.applyShader(_triangleShader, foundation::RenderTopology::TRIANGLES, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.draw(_triangleData, 3);
                rendering.applyShader(_quadShader, foundation::RenderTopology::TRIANGLESTRIP, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.draw(_quadData, 3);
            });
        }
        bool testResult(const API &api, const std::uint8_t *ref, const std::uint8_t *frame) override {
            EXPECT_NOT_EQ(_triangleShader, nullptr);
            EXPECT_NOT_EQ(_quadShader, nullptr);
            EXPECT_NOT_EQ(_triangleData, nullptr);
            EXPECT_NOT_EQ(_quadData, nullptr);

            auto pixelIs = [frame](std::size_t x, std::size_t y, int r, int g, int b) {
                const std::uint8_t *p = frame + (y * foundation::TESTING_RENDER_WIDTH + x) * 4;
                return p[0] == r && p[1] == g && p[2] == b && p[3] == 255;
            };

            EXPECT_TRUE(pixelIs(38, 38, 255, 0, 0));     // triangle instance 0: red
            EXPECT_TRUE(pixelIs(192, 38, 0, 255, 0));    // triangle instance 1: green
            EXPECT_TRUE(pixelIs(346, 38, 0, 0, 255));    // triangle instance 2: blue
            EXPECT_TRUE(pixelIs(115, 115, 0, 0, 0));     // outside triangle instance 0 hypotenuse
            EXPECT_TRUE(pixelIs(423, 115, 0, 0, 0));     // outside triangle instance 2 hypotenuse
            EXPECT_TRUE(pixelIs(480, 38, 0, 0, 0));      // no triangle instance 3
            EXPECT_TRUE(pixelIs(77, 320, 255, 0, 0));    // quad origin 0, instance 0: red
            EXPECT_TRUE(pixelIs(230, 320, 0, 255, 0));   // quad origin 0, instance 1: green
            EXPECT_TRUE(pixelIs(384, 320, 0, 0, 255));   // quad origin 0, instance 2: blue
            EXPECT_TRUE(pixelIs(77, 448, 255, 0, 0));    // quad origin 1, instance 0: red
            EXPECT_TRUE(pixelIs(230, 448, 0, 255, 0));   // quad origin 1, instance 1: green
            EXPECT_TRUE(pixelIs(384, 448, 0, 0, 255));   // quad origin 1, instance 2: blue
            EXPECT_TRUE(pixelIs(154, 320, 0, 0, 0));     // between quad instances 0 and 1
            EXPECT_TRUE(pixelIs(230, 384, 0, 0, 0));     // between quad origins
            EXPECT_TRUE(pixelIs(480, 320, 0, 0, 0));     // no quad instance 3
            return true;
        }

    private:
        foundation::RenderShaderPtr _triangleShader;
        foundation::RenderShaderPtr _quadShader;
        foundation::RenderDataPtr _triangleData;
        foundation::RenderDataPtr _quadData;
    };
}

namespace test {
    class rendering_triple_instancing_test : public PixelTestInterface {
    public:
        static auto make() -> std::unique_ptr<PixelTestInterface> {
            return std::make_unique<rendering_triple_instancing_test>();
        }
        void init(const API &api) override {
            // Triangle from fixed block by repeat_ID, two triangles in group from vertex data,
            // three groups at positions from constants by instance_ID with red, green, blue colors
            //
            const char *shaderSrc = R"(
                fixed {
                    triangle[3] : float2 = [0.0, 0.0][0.3, 0.0][0.0, -0.3]
                    color[3] : float4 = [1.0, 0.0, 0.0, 1.0][0.0, 1.0, 0.0, 1.0][0.0, 0.0, 1.0, 1.0]
                }
                const {
                    positions[3] : float4
                }
                inout {
                    color : float4
                }
                vssrc {
                    output_position = float4(fixed_triangle[repeat_ID] + vertex_position + const_positions[instance_ID].xy, 0.5, 1.0);
                    output_color = fixed_color[instance_ID];
                }
                fssrc {
                    output_color[0] = input_color;
                }
            )";
            const foundation::InputLayout layout = foundation::InputLayout {
                .repeat = 3,
                .attributes = {
                    {"position", foundation::InputAttributeFormat::FLOAT2},
                }
            };
            const float group[] = { 0.0f, 0.0f, 0.4f, 0.0f };
            _shader = api.rendering->createShader(shaderSrc, layout);
            _data = api.rendering->createData(layout, group, 2);
        }
        void updateAndDraw(const API &api) override {
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                const Constants constants = {{
                    { -0.9f, 0.9f, 0.0f, 0.0f },
                    { -0.9f, 0.3f, 0.0f, 0.0f },
                    { -0.9f, -0.3f, 0.0f, 0.0f },
                }};
                rendering.applyShader(_shader, foundation::RenderTopology::TRIANGLES, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.applyShaderConstants(&constants);
                rendering.draw(_data, 3);
            });
        }
        bool testResult(const API &api, const std::uint8_t *ref, const std::uint8_t *frame) override {
            EXPECT_NOT_EQ(_shader, nullptr);
            EXPECT_NOT_EQ(_data, nullptr);

            auto pixelIs = [frame](std::size_t x, std::size_t y, int r, int g, int b) {
                const std::uint8_t *p = frame + (y * foundation::TESTING_RENDER_WIDTH + x) * 4;
                return p[0] == r && p[1] == g && p[2] == b && p[3] == 255;
            };

            EXPECT_TRUE(pixelIs(38, 38, 255, 0, 0));     // group 0, triangle 0: red
            EXPECT_TRUE(pixelIs(141, 38, 255, 0, 0));    // group 0, triangle 1: red
            EXPECT_TRUE(pixelIs(38, 192, 0, 255, 0));    // group 1, triangle 0: green
            EXPECT_TRUE(pixelIs(141, 192, 0, 255, 0));   // group 1, triangle 1: green
            EXPECT_TRUE(pixelIs(38, 346, 0, 0, 255));    // group 2, triangle 0: blue
            EXPECT_TRUE(pixelIs(141, 346, 0, 0, 255));   // group 2, triangle 1: blue
            EXPECT_TRUE(pixelIs(115, 38, 0, 0, 0));      // between triangles
            EXPECT_TRUE(pixelIs(77, 77, 0, 0, 0));       // outside triangle hypotenuse
            EXPECT_TRUE(pixelIs(243, 38, 0, 0, 0));      // no triangle 2
            EXPECT_TRUE(pixelIs(38, 499, 0, 0, 0));      // no group 3
            return true;
        }

    private:
        struct Constants {
            math::vector4f positions[3];
        };

        foundation::RenderShaderPtr _shader;
        foundation::RenderDataPtr _data;
    };
}

namespace test {
    class rendering_4x4_linear_texture_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<rendering_4x4_linear_texture_test>();
        }
        void init(const API &api) override {
            // Fullscreen quad sampling 4x4 multicolor texture with linear filtering
            //
            const char *shaderSrc = R"(
                fixed {
                    quad[4] : float2 = [-1.0, 1.0][1.0, 1.0][-1.0, -1.0][1.0, -1.0]
                    uv[4] : float2 = [0.0, 0.0][1.0, 0.0][0.0, 1.0][1.0, 1.0]
                }
                inout {
                    uv : float2
                }
                vssrc {
                    output_position = float4(fixed_quad[repeat_ID], 0.5, 1.0);
                    output_uv = fixed_uv[repeat_ID];
                }
                fssrc {
                    output_color[0] = _tex2d(0, input_uv);
                }
            )";
            const foundation::InputLayout layout = foundation::InputLayout {
                .repeat = 4
            };
            const std::uint32_t pixels[] = {
                0xff0000ff, 0xff00ff00, 0xffff0000, 0xffffffff,
                0xff00ffff, 0xffffff00, 0xffff00ff, 0xff000000,
                0xff0080ff, 0xff800080, 0xff808080, 0xff808000,
                0xffcbc0ff, 0xff00ff80, 0xff800000, 0xff008080,
            };
            _shader = api.rendering->createShader(shaderSrc, layout);
            _texture = api.rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, 4, 4, { pixels });
        }
        void updateAndDraw(const API &api) override {
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                rendering.applyShader(_shader, foundation::RenderTopology::TRIANGLESTRIP, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.applyTextures({ { _texture, foundation::SamplerType::LINEAR } });
                rendering.draw(1);
            });
        }
        std::uint8_t getThreshold() const override {
            return 4;
        }

        bool testResult(const API &api, std::size_t outOfThresholdPixelCount) override {
            return outOfThresholdPixelCount <= 0;
        }

    private:
        foundation::RenderShaderPtr _shader;
        foundation::RenderTexturePtr _texture;
    };
}

namespace test {
    class rendering_blend_types_test : public PixelTestInterface {
    public:
        static auto make() -> std::unique_ptr<PixelTestInterface> {
            return std::make_unique<rendering_blend_types_test>();
        }
        void init(const API &api) override {
            // Overlapping semi-transparent red and blue quads: mixing on the top half, additive on the bottom half
            //
            const char *shaderSrc = R"(
                fixed {
                    quad[4] : float2 = [0.0, 0.8][0.8, 0.8][0.0, 0.0][0.8, 0.0]
                }
                const {
                    offset : float4
                    color : float4
                }
                vssrc {
                    output_position = float4(fixed_quad[repeat_ID], 0.5, 1.0) + const_offset;
                }
                fssrc {
                    output_color[0] = const_color;
                }
            )";
            const foundation::InputLayout layout = foundation::InputLayout {
                .repeat = 4
            };
            _shader = api.rendering->createShader(shaderSrc, layout);
        }
        void updateAndDraw(const API &api) override {
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                drawPair(rendering, foundation::BlendType::MIXING, 0.1f);
                drawPair(rendering, foundation::BlendType::ADDITIVE, -0.9f);
            });
        }
        bool testResult(const API &api, const std::uint8_t *ref, const std::uint8_t *frame) override {
            EXPECT_NOT_EQ(_shader, nullptr);

            EXPECT_TRUE(isPixel(frame, 77, 128, 128, 0));     // mixing: red only
            EXPECT_TRUE(isPixel(frame, 179, 128, 64, 128));   // mixing: blue over red
            EXPECT_TRUE(isPixel(frame, 282, 128, 0, 128));    // mixing: blue only
            EXPECT_TRUE(isPixel(frame, 77, 384, 128, 0));     // additive: red only
            EXPECT_TRUE(isPixel(frame, 179, 384, 128, 128));  // additive: blue plus red
            EXPECT_TRUE(isPixel(frame, 282, 384, 0, 128));    // additive: blue only
            EXPECT_TRUE(isPixel(frame, 450, 128, 0, 0));      // empty
            return true;
        }

    private:
        struct Constants {
            math::vector4f offset;
            math::vector4f color;
        };

        void drawPair(foundation::RenderingInterface &rendering, foundation::BlendType blendType, float y) {
            const Constants red = { { -0.9f, y, 0.0f, 0.0f }, { 1.0f, 0.0f, 0.0f, 0.5f } };
            const Constants blue = { { -0.5f, y, 0.0f, 0.0f }, { 0.0f, 0.0f, 1.0f, 0.5f } };

            rendering.applyShader(_shader, foundation::RenderTopology::TRIANGLESTRIP, blendType, foundation::DepthBehavior::DISABLED);
            rendering.applyShaderConstants(&red);
            rendering.draw(1);
            rendering.applyShaderConstants(&blue);
            rendering.draw(1);
        }

        static bool isPixel(const std::uint8_t *frame, std::size_t x, std::size_t y, int r, int b) {
            const std::uint8_t *pixel = frame + (y * foundation::TESTING_RENDER_WIDTH + x) * 4;
            return std::abs(pixel[0] - r) < 3 && pixel[1] == 0 && std::abs(pixel[2] - b) < 3;
        }

        foundation::RenderShaderPtr _shader;
    };
}

namespace test {
    class rendering_cube_texture_test : public PixelTestInterface {
    public:
        static auto make() -> std::unique_ptr<PixelTestInterface> {
            return std::make_unique<rendering_cube_texture_test>();
        }
        void init(const API &api) override {
            // Sphere sampling 1x1 cube texture by normal: red +x, green -x, blue +y, yellow -y, magenta +z, cyan -z. Back half is discarded
            //
            const char *shaderSrc = R"(
                inout {
                    normal : float3
                }
                vssrc {
                    output_position = float4(vertex_position.xy * 0.8, 0.5, 1.0);
                    output_normal = vertex_position;
                }
                fssrc [cube] {
                    if (input_normal.z < 0.0) {
                        _discard();
                    }
                    output_color[0] = _texcube(0, input_normal);
                }
            )";
            const foundation::InputLayout layout = foundation::InputLayout {
                .attributes = {
                    {"position", foundation::InputAttributeFormat::FLOAT3},
                }
            };
            const std::uint32_t faces[] = { 0xff0000ff, 0xff00ff00, 0xffff0000, 0xff00ffff, 0xffff00ff, 0xffffff00 };
            const std::uint32_t rings = 16;
            const std::uint32_t segments = 32;

            std::vector<math::vector3f> vertexes;
            std::vector<std::uint32_t> indexes;

            for (std::uint32_t r = 0; r <= rings; r++) {
                const float theta = MATH_PI * float(r) / float(rings);
                for (std::uint32_t s = 0; s <= segments; s++) {
                    const float phi = 2.0f * MATH_PI * float(s) / float(segments);
                    vertexes.emplace_back(std::sin(theta) * std::cos(phi), std::cos(theta), std::sin(theta) * std::sin(phi));
                }
            }
            for (std::uint32_t r = 0; r < rings; r++) {
                for (std::uint32_t s = 0; s < segments; s++) {
                    const std::uint32_t i = r * (segments + 1) + s;
                    indexes.insert(indexes.end(), { i, i + segments + 1, i + 1, i + 1, i + segments + 1, i + segments + 2 });
                }
            }

            _shader = api.rendering->createShader(shaderSrc, layout);
            const void *faceData[] = { &faces[0], &faces[1], &faces[2], &faces[3], &faces[4], &faces[5] };
            _texture = api.rendering->createCubeTexture(foundation::RenderTextureFormat::RGBA8UN, 1, 1, faceData);
            _data = api.rendering->createData(layout, vertexes.data(), std::uint32_t(vertexes.size()), indexes.data(), std::uint32_t(indexes.size()));
        }
        void updateAndDraw(const API &api) override {
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                rendering.applyShader(_shader, foundation::RenderTopology::TRIANGLES, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.applyTextures({ { _texture, foundation::SamplerType::NEAREST } });
                rendering.draw(_data);
            });
        }
        bool testResult(const API &api, const std::uint8_t *ref, const std::uint8_t *frame) override {
            EXPECT_NOT_EQ(_shader, nullptr);
            EXPECT_NOT_EQ(_texture, nullptr);
            EXPECT_NOT_EQ(_data, nullptr);
            EXPECT_EQ(_texture->getMipCount(), 1);

            auto pixelIs = [frame](std::size_t x, std::size_t y, int r, int g, int b) {
                const std::uint8_t *p = frame + (y * foundation::TESTING_RENDER_WIDTH + x) * 4;
                return p[0] == r && p[1] == g && p[2] == b && p[3] == 255;
            };

            EXPECT_TRUE(pixelIs(256, 256, 255, 0, 255));    // center: +z
            EXPECT_TRUE(pixelIs(436, 256, 255, 0, 0));      // right: +x
            EXPECT_TRUE(pixelIs(76, 256, 0, 255, 0));       // left: -x
            EXPECT_TRUE(pixelIs(256, 76, 0, 0, 255));       // top: +y
            EXPECT_TRUE(pixelIs(256, 436, 255, 255, 0));    // bottom: -y
            EXPECT_TRUE(pixelIs(20, 20, 0, 0, 0));          // outside
            return true;
        }

    private:
        foundation::RenderShaderPtr _shader;
        foundation::RenderTexturePtr _texture;
        foundation::RenderDataPtr _data;
    };
}

namespace test {
    class rendering_depth_behavior_test : public PixelTestInterface {
    public:
        static auto make() -> std::unique_ptr<PixelTestInterface> {
            return std::make_unique<rendering_depth_behavior_test>();
        }
        void init(const API &api) override {
            // Near red and far green quads overlapping in each quadrant of the target with z-buffer, then target texture drawn as fullscreen quad
            //
            const char *quadSrc = R"(
                fixed {
                    quad[4] : float2 = [0.0, 0.0][1.0, 0.0][0.0, -1.0][1.0, -1.0]
                }
                const {
                    offset : float4
                    color : float4
                }
                vssrc {
                    output_position = float4(fixed_quad[repeat_ID] + const_offset.xy, const_offset.z, 1.0);
                }
                fssrc {
                    output_color[0] = const_color;
                }
            )";
            const char *screenSrc = R"(
                fixed {
                    quad[4] : float2 = [-1.0, 1.0][1.0, 1.0][-1.0, -1.0][1.0, -1.0]
                    uv[4] : float2 = [0.0, 0.0][1.0, 0.0][0.0, 1.0][1.0, 1.0]
                }
                inout {
                    uv : float2
                }
                vssrc {
                    output_position = float4(fixed_quad[repeat_ID], 0.5, 1.0);
                    output_uv = fixed_uv[repeat_ID];
                }
                fssrc {
                    output_color[0] = _tex2d(0, input_uv);
                }
            )";
            _quadShader = api.rendering->createShader(quadSrc, foundation::InputLayout { .repeat = 4 });
            _screenShader = api.rendering->createShader(screenSrc, foundation::InputLayout { .repeat = 4 });
            _target = api.rendering->createRenderTarget(foundation::RenderTextureFormat::RGBA8UN, 1, 64, 64, true);
        }
        void updateAndDraw(const API &api) override {
            using foundation::DepthBehavior;

            api.rendering->forTarget(_target, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                drawQuad(rendering, DepthBehavior::TEST_AND_WRITE, -1.0f, 1.0f, true);
                drawQuad(rendering, DepthBehavior::TEST_AND_WRITE, -1.0f, 1.0f, false);
                drawQuad(rendering, DepthBehavior::TEST_AND_WRITE, -1.0f, 0.0f, false);
                drawQuad(rendering, DepthBehavior::TEST_AND_WRITE, -1.0f, 0.0f, true);
                drawQuad(rendering, DepthBehavior::TEST_ONLY, 0.0f, 1.0f, true);
                drawQuad(rendering, DepthBehavior::TEST_AND_WRITE, 0.0f, 1.0f, false);
                drawQuad(rendering, DepthBehavior::TEST_AND_WRITE, 0.0f, 0.0f, true);
                drawQuad(rendering, DepthBehavior::TEST_ONLY, 0.0f, 0.0f, false);
            });
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                rendering.applyShader(_screenShader, foundation::RenderTopology::TRIANGLESTRIP, foundation::BlendType::DISABLED, DepthBehavior::DISABLED);
                rendering.applyTextures({ { _target->getTexture(0), foundation::SamplerType::NEAREST } });
                rendering.draw(1);
            });
        }
        bool testResult(const API &api, const std::uint8_t *ref, const std::uint8_t *frame) override {
            EXPECT_NOT_EQ(_quadShader, nullptr);
            EXPECT_NOT_EQ(_screenShader, nullptr);
            EXPECT_NOT_EQ(_target, nullptr);

            auto pixelIs = [frame](std::size_t x, std::size_t y, int r, int g) {
                const std::uint8_t *p = frame + (y * foundation::TESTING_RENDER_WIDTH + x) * 4;
                return p[0] == r && p[1] == g && p[2] == 0 && p[3] == 255;
            };

            const std::size_t w = foundation::TESTING_RENDER_WIDTH;
            const std::size_t h = foundation::TESTING_RENDER_HEIGHT;

            EXPECT_TRUE(pixelIs(w / 4, h / 4, 255, 0));           // test and write: near drawn first stays visible
            EXPECT_TRUE(pixelIs(w / 4, h * 3 / 4, 255, 0));       // test and write: near drawn last overwrites far
            EXPECT_TRUE(pixelIs(w * 3 / 4, h / 4, 0, 255));       // test only: near doesn't write depth, far overwrites it
            EXPECT_TRUE(pixelIs(w * 3 / 4, h * 3 / 4, 255, 0));   // test only: far fails depth test against near
            return true;
        }

    private:
        struct Constants {
            math::vector4f offset;
            math::vector4f color;
        };

        // near quad is red, far quad is green
        //
        void drawQuad(foundation::RenderingInterface &rendering, foundation::DepthBehavior depthBehavior, float x, float y, bool isNear) {
            const Constants constants = { { x, y, isNear ? 0.8f : 0.2f, 0.0f }, { isNear ? 1.0f : 0.0f, isNear ? 0.0f : 1.0f, 0.0f, 1.0f } };
            rendering.applyShader(_quadShader, foundation::RenderTopology::TRIANGLESTRIP, foundation::BlendType::DISABLED, depthBehavior);
            rendering.applyShaderConstants(&constants);
            rendering.draw(1);
        }

        foundation::RenderShaderPtr _quadShader;
        foundation::RenderShaderPtr _screenShader;
        foundation::RenderTargetPtr _target;
    };
}

namespace test {
    class rendering_partial_data_test : public PixelTestInterface {
    public:
        static auto make() -> std::unique_ptr<PixelTestInterface> {
            return std::make_unique<rendering_partial_data_test>();
        }
        void init(const API &api) override {
            // Four white triangles in a row drawn partially: top row by vertexes 3..8, middle row by reversed indexes from 6 to the end,
            // four quads with repeat layout in the bottom row drawn partially from instance 1 to 2
            //
            const char *triangleSrc = R"(
                vssrc {
                    output_position = float4(vertex_position, 0.5, 1.0);
                }
                fssrc {
                    output_color[0] = float4(1.0, 1.0, 1.0, 1.0);
                }
            )";
            const char *quadSrc = R"(
                fixed {
                    quad[4] : float2 = [0.0, 0.0][0.3, 0.0][0.0, -0.3][0.3, -0.3]
                }
                vssrc {
                    output_position = float4(vertex_position + fixed_quad[repeat_ID], 0.5, 1.0);
                }
                fssrc {
                    output_color[0] = float4(1.0, 1.0, 1.0, 1.0);
                }
            )";
            const foundation::InputLayout triangleLayout = foundation::InputLayout {
                .attributes = {
                    {"position", foundation::InputAttributeFormat::FLOAT2},
                }
            };
            const foundation::InputLayout quadLayout = foundation::InputLayout {
                .repeat = 4,
                .attributes = {
                    {"position", foundation::InputAttributeFormat::FLOAT2},
                }
            };
            const std::uint32_t indexes[] = { 9, 10, 11, 6, 7, 8, 3, 4, 5, 0, 1, 2 };
            std::vector<math::vector2f> top, middle, origins;

            for (std::uint32_t i = 0; i < 4; i++) {
                const float x = -0.9f + 0.45f * float(i);
                top.insert(top.end(), { {x, 0.9f}, {x + 0.3f, 0.9f}, {x, 0.6f} });
                middle.insert(middle.end(), { {x, 0.3f}, {x + 0.3f, 0.3f}, {x, 0.0f} });
                origins.emplace_back(x, -0.3f);
            }

            _triangleShader = api.rendering->createShader(triangleSrc, triangleLayout);
            _quadShader = api.rendering->createShader(quadSrc, quadLayout);
            _topData = api.rendering->createData(triangleLayout, top.data(), std::uint32_t(top.size()));
            _middleData = api.rendering->createData(triangleLayout, middle.data(), std::uint32_t(middle.size()), indexes, 12);
            _quadData = api.rendering->createData(quadLayout, origins.data(), std::uint32_t(origins.size()));
        }
        void updateAndDraw(const API &api) override {
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                rendering.applyShader(_triangleShader, foundation::RenderTopology::TRIANGLES, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.draw(_topData, 1, 3, 6);
                rendering.draw(_middleData, 1, 6);
                rendering.applyShader(_quadShader, foundation::RenderTopology::TRIANGLESTRIP, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.draw(_quadData, 1, 1, 2);
            });
        }
        bool testResult(const API &api, const std::uint8_t *ref, const std::uint8_t *frame) override {
            EXPECT_NOT_EQ(_triangleShader, nullptr);
            EXPECT_NOT_EQ(_quadShader, nullptr);
            EXPECT_NOT_EQ(_topData, nullptr);
            EXPECT_NOT_EQ(_middleData, nullptr);
            EXPECT_NOT_EQ(_quadData, nullptr);

            auto isWhite = [frame](std::size_t x, std::size_t y) {
                const std::uint8_t *p = frame + (y * foundation::TESTING_RENDER_WIDTH + x) * 4;
                return p[0] == 255 && p[1] == 255 && p[2] == 255;
            };

            EXPECT_FALSE(isWhite(46, 46));      // top triangle 0: before start
            EXPECT_TRUE(isWhite(161, 46));      // top triangle 1
            EXPECT_TRUE(isWhite(276, 46));      // top triangle 2
            EXPECT_FALSE(isWhite(391, 46));     // top triangle 3: after count
            EXPECT_TRUE(isWhite(46, 200));      // middle triangle 0: last indexes
            EXPECT_TRUE(isWhite(161, 200));     // middle triangle 1
            EXPECT_FALSE(isWhite(276, 200));    // middle triangle 2: before start
            EXPECT_FALSE(isWhite(391, 200));    // middle triangle 3: before start
            EXPECT_FALSE(isWhite(64, 371));     // quad 0: before start
            EXPECT_TRUE(isWhite(179, 371));     // quad 1
            EXPECT_TRUE(isWhite(294, 371));     // quad 2
            EXPECT_FALSE(isWhite(410, 371));    // quad 3: after count
            return true;
        }

    private:
        foundation::RenderShaderPtr _triangleShader;
        foundation::RenderShaderPtr _quadShader;
        foundation::RenderDataPtr _topData;
        foundation::RenderDataPtr _middleData;
        foundation::RenderDataPtr _quadData;
    };
}

namespace test {
    class rendering_indexed_data_test : public PixelTestInterface {
    public:
        static auto make() -> std::unique_ptr<PixelTestInterface> {
            return std::make_unique<rendering_indexed_data_test>();
        }
        void init(const API &api) override {
            // Two indexed quads from 4 vertexes and 6 indexes: red in the top-left from data, green in the bottom-right from dynamic draw
            //
            const char *shaderSrc = R"(
                const {
                    color : float4
                }
                vssrc {
                    output_position = float4(vertex_position, 0.5, 1.0);
                }
                fssrc {
                    output_color[0] = const_color;
                }
            )";
            const float red[] = { -0.9f, 0.9f, -0.1f, 0.9f, -0.9f, 0.1f, -0.1f, 0.1f };
            _shader = api.rendering->createShader(shaderSrc, _layout);
            _data = api.rendering->createData(_layout, red, 4, _indexes, 6);
        }
        void updateAndDraw(const API &api) override {
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                const math::vector4f redColor = { 1.0f, 0.0f, 0.0f, 1.0f };
                const math::vector4f greenColor = { 0.0f, 1.0f, 0.0f, 1.0f };
                const float green[] = { 0.1f, -0.1f, 0.9f, -0.1f, 0.1f, -0.9f, 0.9f, -0.9f };

                rendering.applyShader(_shader, foundation::RenderTopology::TRIANGLES, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.applyShaderConstants(&redColor);
                rendering.draw(_data);
                rendering.applyShaderConstants(&greenColor);
                rendering.draw(green, 4, _indexes, 6);
            });
        }
        bool testResult(const API &api, const std::uint8_t *ref, const std::uint8_t *frame) override {
            EXPECT_NOT_EQ(_shader, nullptr);
            EXPECT_NOT_EQ(_data, nullptr);
            EXPECT_EQ(_data->getVertexCount(), 4);
            EXPECT_EQ(_data->getIndexCount(), 6);

            auto pixelIs = [frame](std::size_t x, std::size_t y, int r, int g) {
                const std::uint8_t *p = frame + (y * foundation::TESTING_RENDER_WIDTH + x) * 4;
                return p[0] == r && p[1] == g && p[2] == 0 && p[3] == 255;
            };

            EXPECT_TRUE(pixelIs(35, 35, 255, 0));    // red quad: top-left (1st triangle)
            EXPECT_TRUE(pixelIs(220, 35, 255, 0));   // red quad: top-right
            EXPECT_TRUE(pixelIs(35, 220, 255, 0));   // red quad: bottom-left
            EXPECT_TRUE(pixelIs(220, 220, 255, 0));  // red quad: bottom-right (2nd triangle)
            EXPECT_TRUE(pixelIs(291, 291, 0, 255));  // green quad: top-left (1st triangle)
            EXPECT_TRUE(pixelIs(476, 291, 0, 255));  // green quad: top-right
            EXPECT_TRUE(pixelIs(291, 476, 0, 255));  // green quad: bottom-left
            EXPECT_TRUE(pixelIs(476, 476, 0, 255));  // green quad: bottom-right (2nd triangle)
            EXPECT_TRUE(pixelIs(240, 128, 0, 0));    // outside red quad: right
            EXPECT_TRUE(pixelIs(384, 496, 0, 0));    // outside green quad: bottom
            EXPECT_TRUE(pixelIs(384, 128, 0, 0));    // empty top-right corner
            EXPECT_TRUE(pixelIs(128, 384, 0, 0));    // empty bottom-left corner
            return true;
        }

    private:
        const foundation::InputLayout _layout = foundation::InputLayout {
            .attributes = {
                {"position", foundation::InputAttributeFormat::FLOAT2},
            }
        };
        const std::uint32_t _indexes[6] = { 0, 1, 2, 2, 1, 3 };

        foundation::RenderShaderPtr _shader;
        foundation::RenderDataPtr _data;
    };
}

namespace test {
    class rendering_points_and_line_strip_test : public PixelTestInterface {
    public:
        static auto make() -> std::unique_ptr<PixelTestInterface> {
            return std::make_unique<rendering_points_and_line_strip_test>();
        }
        void init(const API &api) override {
            // White strip 'Г' in the top-left quarter
            //
            const char *stripSrc = R"(
                fixed {
                    strip[3] : float2 = [-0.9, 0.7][-0.3, 0.7][-0.3, 0.1]
                }
                vssrc {
                    output_position = float4(fixed_strip[repeat_ID], 0.5, 1.0);
                }
                fssrc {
                    output_color[0] = float4(1.0, 1.0, 1.0, 1.0);
                }
            )";
            // White points at pixel centers (320, 64), (384, 64), (448, 64)
            //
            const char *pointsSrc = R"(
                fixed {
                    points[3] : float2 = [0.251953125, 0.748046875][0.501953125, 0.748046875][0.751953125, 0.748046875]
                }
                vssrc {
                    output_position = float4(fixed_points[vertex_ID], 0.5, 1.0);
                }
                fssrc {
                    output_color[0] = float4(1.0, 1.0, 1.0, 1.0);
                }
            )";
            const char *dynamicSrc = R"(
                const {
                    color : float4
                }
                vssrc {
                    output_position = float4(vertex_position, 0.5, 1.0);
                }
                fssrc {
                    output_color[0] = const_color;
                }
            )";
            _stripShader = api.rendering->createShader(stripSrc, foundation::InputLayout { .repeat = 3 });
            _pointsShader = api.rendering->createShader(pointsSrc, foundation::InputLayout {});
            _dynamicShader = api.rendering->createShader(dynamicSrc, _layout);
        }
        void updateAndDraw(const API &api) override {
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                const math::vector4f red = { 1.0f, 0.0f, 0.0f, 1.0f };
                const math::vector4f green = { 0.0f, 1.0f, 0.0f, 1.0f };
                const float strip[] = { -0.9f, -0.3f, -0.3f, -0.3f, -0.3f, -0.9f };
                const float points[] = { 0.251953125f, -0.751953125f, 0.501953125f, -0.751953125f, 0.751953125f, -0.751953125f };

                rendering.applyShader(_stripShader, foundation::RenderTopology::LINESTRIP, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.draw(1);
                rendering.applyShader(_pointsShader, foundation::RenderTopology::POINTS, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.draw(3);
                rendering.applyShader(_dynamicShader, foundation::RenderTopology::LINESTRIP, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.applyShaderConstants(&red);
                rendering.draw(strip, 3);
                rendering.applyShader(_dynamicShader, foundation::RenderTopology::POINTS, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.applyShaderConstants(&green);
                rendering.draw(points, 3);
            });
        }
        bool testResult(const API &api, const std::uint8_t *ref, const std::uint8_t *frame) override {
            EXPECT_NOT_EQ(_stripShader, nullptr);
            EXPECT_NOT_EQ(_pointsShader, nullptr);
            EXPECT_NOT_EQ(_dynamicShader, nullptr);

            auto pixelIs = [frame](std::size_t x, std::size_t y, int r, int g, int b) {
                const std::uint8_t *p = frame + (y * foundation::TESTING_RENDER_WIDTH + x) * 4;
                return p[0] == r && p[1] == g && p[2] == b && p[3] == 255;
            };

            EXPECT_TRUE(pixelIs(100, 76, 255, 255, 255));   // fixed strip: 1st segment
            EXPECT_TRUE(pixelIs(179, 150, 255, 255, 255));  // fixed strip: 2nd segment
            EXPECT_TRUE(pixelIs(102, 153, 0, 0, 0));        // fixed strip: no closing segment
            EXPECT_TRUE(pixelIs(100, 90, 0, 0, 0));         // fixed strip: below 1st segment
            EXPECT_TRUE(pixelIs(320, 64, 255, 255, 255));   // fixed points: 1st
            EXPECT_TRUE(pixelIs(384, 64, 255, 255, 255));   // fixed points: 2nd
            EXPECT_TRUE(pixelIs(448, 64, 255, 255, 255));   // fixed points: 3rd
            EXPECT_TRUE(pixelIs(321, 64, 0, 0, 0));         // fixed points: right of 1st
            EXPECT_TRUE(pixelIs(384, 65, 0, 0, 0));         // fixed points: below 2nd
            EXPECT_TRUE(pixelIs(416, 64, 0, 0, 0));         // fixed points: between 2nd and 3rd
            EXPECT_TRUE(pixelIs(100, 332, 255, 0, 0));      // dynamic strip: 1st segment
            EXPECT_TRUE(pixelIs(179, 410, 255, 0, 0));      // dynamic strip: 2nd segment
            EXPECT_TRUE(pixelIs(102, 409, 0, 0, 0));        // dynamic strip: no closing segment
            EXPECT_TRUE(pixelIs(320, 448, 0, 255, 0));      // dynamic points: 1st
            EXPECT_TRUE(pixelIs(384, 448, 0, 255, 0));      // dynamic points: 2nd
            EXPECT_TRUE(pixelIs(448, 448, 0, 255, 0));      // dynamic points: 3rd
            EXPECT_TRUE(pixelIs(447, 448, 0, 0, 0));        // dynamic points: left of 3rd
            EXPECT_TRUE(pixelIs(320, 447, 0, 0, 0));        // dynamic points: above 1st
            EXPECT_TRUE(pixelIs(352, 448, 0, 0, 0));        // dynamic points: between 1st and 2nd
            return true;
        }

    private:
        const foundation::InputLayout _layout = foundation::InputLayout {
            .attributes = {
                {"position", foundation::InputAttributeFormat::FLOAT2},
            }
        };

        foundation::RenderShaderPtr _stripShader;
        foundation::RenderShaderPtr _pointsShader;
        foundation::RenderShaderPtr _dynamicShader;
    };
}

namespace test {
    bool rendering_vertex_attribute_stride_test(const API &api) {
        using F = foundation::InputAttributeFormat;
        auto stride = [](std::initializer_list<F> formats) {
            foundation::InputLayout layout;
            for (F format : formats) {
                layout.attributes.push_back({"attribute", format});
            }
            return layout.getStride();
        };

        EXPECT_EQ(stride({}), 0);
        EXPECT_EQ(stride({F::HALF2, F::HALF4}), 12);
        EXPECT_EQ(stride({F::FLOAT1, F::FLOAT2, F::FLOAT3, F::FLOAT4}), 40);
        EXPECT_EQ(stride({F::SHORT2, F::SHORT4, F::USHORT2, F::USHORT4}), 24);
        EXPECT_EQ(stride({F::SHORT2_NRM, F::SHORT4_NRM, F::USHORT2_NRM, F::USHORT4_NRM}), 24);
        EXPECT_EQ(stride({F::BYTE4, F::BYTE4_NRM}), 8);
        EXPECT_EQ(stride({F::INT1, F::INT2, F::INT3, F::INT4}), 40);
        EXPECT_EQ(stride({F::UINT1, F::UINT2, F::UINT3, F::UINT4}), 40);
        return true;
    }
}

namespace test {
    class rendering_vertex_attribute_formats_test : public PixelTestInterface {
    public:
        static auto make() -> std::unique_ptr<PixelTestInterface> {
            return std::make_unique<rendering_vertex_attribute_formats_test>();
        }
        void init(const API &api) override {
            // 3x3 grid of quads, each instance checks one non-float attribute of a single vertex: green if decoded value is expected, red otherwise
            //
            const char *shaderSrc = R"(
                fixed {
                    quad[4] : float2 = [0.0, 0.0][0.5, 0.0][0.0, -0.5][0.5, -0.5]
                    offset[9] : float2 = [-0.9, 0.9][-0.3, 0.9][0.3, 0.9][-0.9, 0.3][-0.3, 0.3][0.3, 0.3][-0.9, -0.3][-0.3, -0.3][0.3, -0.3]
                }
                inout {
                    color : float4
                }
                vssrc {
                    int index = int(instance_ID);
                    float4 diff = float4(0.0, 0.0, 0.0, 0.0);
                    if (index == 0) { diff = vertex_h4 - float4(0.5, -0.25, 2.0, -8.0); }
                    if (index == 1) { diff = float4(float2(vertex_s2) - float2(-300.0, 200.0), 0.0, 0.0); }
                    if (index == 2) { diff = float4(vertex_us4) - float4(1.0, 1000.0, 40000.0, 65535.0); }
                    if (index == 3) { diff = vertex_s4n - float4(1.0, -1.0, 0.0, 0.5); }
                    if (index == 4) { diff = float4(vertex_us2n - float2(1.0, 0.2), 0.0, 0.0); }
                    if (index == 5) { diff = float4(vertex_b4) - float4(1.0, 2.0, 128.0, 255.0); }
                    if (index == 6) { diff = vertex_b4n - float4(1.0, 0.0, 0.2, 0.8); }
                    if (index == 7) { diff = float4(float(vertex_i1) + 100000.0, 0.0, 0.0, 0.0); }
                    if (index == 8) { diff = float4(float2(vertex_u2) - float2(3000000000.0, 7.0), 0.0, 0.0); }
                    float err = _dot(_abs(diff), float4(1.0, 1.0, 1.0, 1.0));
                    output_position = float4(fixed_quad[repeat_ID] + fixed_offset[instance_ID], 0.5, 1.0);
                    output_color = _select(float4(1.0, 0.0, 0.0, 1.0), float4(0.0, 1.0, 0.0, 1.0), err < 0.01);
                }
                fssrc {
                    output_color[0] = input_color;
                }
            )";
            const Vertex vertex = {
                { 0x3800, 0xB400, 0x4000, 0xC800 },
                { -300, 200 },
                { 1, 1000, 40000, 65535 },
                { 32767, -32767, 0, 16384 },
                { 65535, 13107 },
                { 1, 2, 128, 255 },
                { 255, 0, 51, 204 },
                -100000,
                { 3000000000u, 7 },
            };
            _shader = api.rendering->createShader(shaderSrc, _layout);
            _data = api.rendering->createData(_layout, &vertex, 1);
        }
        void updateAndDraw(const API &api) override {
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                rendering.applyShader(_shader, foundation::RenderTopology::TRIANGLESTRIP, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.draw(_data, 9);
            });
        }
        bool testResult(const API &api, const std::uint8_t *ref, const std::uint8_t *frame) override {
            EXPECT_NOT_EQ(_shader, nullptr);
            EXPECT_NOT_EQ(_data, nullptr);
            EXPECT_EQ(_layout.getStride(), sizeof(Vertex));
            EXPECT_EQ(_data->getStride(), sizeof(Vertex));

            auto pixelIs = [frame](std::size_t x, std::size_t y, int r, int g) {
                const std::uint8_t *p = frame + (y * foundation::TESTING_RENDER_WIDTH + x) * 4;
                return p[0] == r && p[1] == g && p[2] == 0 && p[3] == 255;
            };

            EXPECT_TRUE(pixelIs(90, 90, 0, 255));     // HALF4
            EXPECT_TRUE(pixelIs(243, 90, 0, 255));    // SHORT2
            EXPECT_TRUE(pixelIs(397, 90, 0, 255));    // USHORT4
            EXPECT_TRUE(pixelIs(90, 243, 0, 255));    // SHORT4_NRM
            EXPECT_TRUE(pixelIs(243, 243, 0, 255));   // USHORT2_NRM
            EXPECT_TRUE(pixelIs(397, 243, 0, 255));   // BYTE4
            EXPECT_TRUE(pixelIs(90, 397, 0, 255));    // BYTE4_NRM
            EXPECT_TRUE(pixelIs(243, 397, 0, 255));   // INT1
            EXPECT_TRUE(pixelIs(397, 397, 0, 255));   // UINT2
            EXPECT_TRUE(pixelIs(490, 490, 0, 0));     // empty
            return true;
        }

    private:
        struct Vertex {
            std::uint16_t h4[4];
            std::int16_t s2[2];
            std::uint16_t us4[4];
            std::int16_t s4n[4];
            std::uint16_t us2n[2];
            std::uint8_t b4[4];
            std::uint8_t b4n[4];
            std::int32_t i1;
            std::uint32_t u2[2];
        };

        const foundation::InputLayout _layout = foundation::InputLayout {
            .repeat = 4,
            .attributes = {
                {"h4", foundation::InputAttributeFormat::HALF4},
                {"s2", foundation::InputAttributeFormat::SHORT2},
                {"us4", foundation::InputAttributeFormat::USHORT4},
                {"s4n", foundation::InputAttributeFormat::SHORT4_NRM},
                {"us2n", foundation::InputAttributeFormat::USHORT2_NRM},
                {"b4", foundation::InputAttributeFormat::BYTE4},
                {"b4n", foundation::InputAttributeFormat::BYTE4_NRM},
                {"i1", foundation::InputAttributeFormat::INT1},
                {"u2", foundation::InputAttributeFormat::UINT2},
            }
        };

        foundation::RenderShaderPtr _shader;
        foundation::RenderDataPtr _data;
    };
}

namespace test {
    class rendering_multiple_render_targets_test : public PixelTestInterface {
    public:
        static auto make() -> std::unique_ptr<PixelTestInterface> {
            return std::make_unique<rendering_multiple_render_targets_test>();
        }
        void init(const API &api) override {
            // Fullscreen quad writes red, green, blue, white into four target textures, then each texture drawn into its own screen quarter
            //
            const char *fillSrc = R"(
                fixed {
                    quad[4] : float2 = [-1.0, 1.0][1.0, 1.0][-1.0, -1.0][1.0, -1.0]
                }
                vssrc {
                    output_position = float4(fixed_quad[repeat_ID], 0.5, 1.0);
                }
                fssrc {
                    output_color[0] = float4(1.0, 0.0, 0.0, 1.0);
                    output_color[1] = float4(0.0, 1.0, 0.0, 1.0);
                    output_color[2] = float4(0.0, 0.0, 1.0, 1.0);
                    output_color[3] = float4(1.0, 1.0, 1.0, 1.0);
                }
            )";
            const char *quarterSrc = R"(
                fixed {
                    quad[4] : float2 = [0.0, 0.0][1.0, 0.0][0.0, -1.0][1.0, -1.0]
                }
                const {
                    offset : float4
                }
                vssrc {
                    output_position = float4(fixed_quad[repeat_ID] + const_offset.xy, 0.5, 1.0);
                }
                fssrc {
                    output_color[0] = _tex2d(0, fragment_coord);
                }
            )";
            _fillShader = api.rendering->createShader(fillSrc, foundation::InputLayout { .repeat = 4 });
            _quarterShader = api.rendering->createShader(quarterSrc, foundation::InputLayout { .repeat = 4 });
            _target = api.rendering->createRenderTarget(foundation::RenderTextureFormat::RGBA8UN, 4, 64, 64, false);
        }
        void updateAndDraw(const API &api) override {
            api.rendering->forTarget(_target, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                rendering.applyShader(_fillShader, foundation::RenderTopology::TRIANGLESTRIP, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.draw(1);
            });
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                const math::vector4f offsets[4] = { { -1.0f, 1.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f, 0.0f } };
                rendering.applyShader(_quarterShader, foundation::RenderTopology::TRIANGLESTRIP, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);

                for (std::uint32_t i = 0; i < 4; i++) {
                    rendering.applyShaderConstants(&offsets[i]);
                    rendering.applyTextures({ { _target->getTexture(i), foundation::SamplerType::NEAREST } });
                    rendering.draw(1);
                }
            });
        }
        bool testResult(const API &api, const std::uint8_t *ref, const std::uint8_t *frame) override {
            EXPECT_NOT_EQ(_fillShader, nullptr);
            EXPECT_NOT_EQ(_quarterShader, nullptr);
            EXPECT_NOT_EQ(_target, nullptr);
            EXPECT_EQ(_target->getTextureCount(), 4);

            auto pixelIs = [frame](std::size_t x, std::size_t y, int r, int g, int b) {
                const std::uint8_t *p = frame + (y * foundation::TESTING_RENDER_WIDTH + x) * 4;
                return p[0] == r && p[1] == g && p[2] == b && p[3] == 255;
            };

            const std::size_t w = foundation::TESTING_RENDER_WIDTH;
            const std::size_t h = foundation::TESTING_RENDER_HEIGHT;

            EXPECT_TRUE(pixelIs(w / 4, h / 4, 255, 0, 0));              // texture 0: red
            EXPECT_TRUE(pixelIs(w * 3 / 4, h / 4, 0, 255, 0));          // texture 1: green
            EXPECT_TRUE(pixelIs(w / 4, h * 3 / 4, 0, 0, 255));          // texture 2: blue
            EXPECT_TRUE(pixelIs(w * 3 / 4, h * 3 / 4, 255, 255, 255));  // texture 3: white
            return true;
        }

    private:
        foundation::RenderShaderPtr _fillShader;
        foundation::RenderShaderPtr _quarterShader;
        foundation::RenderTargetPtr _target;
    };
}

namespace test {
    void registerRenderingTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(rendering_uniquty_test);
        REGISTER_UNIT_TEST(rendering_back_buffer_size_test);
        REGISTER_UNIT_TEST(rendering_statistics_test);
        REGISTER_UNIT_TEST(rendering_vertex_attribute_stride_test);
        REGISTER_PIXEL_TEST("rendering_tests", rendering_fill_green_test);
        REGISTER_PIXEL_TEST("rendering_tests", rendering_white_line_on_black_test);
        REGISTER_PIXEL_TEST("rendering_tests", rendering_red_triangle_on_black_test);
        REGISTER_PIXEL_TEST("rendering_tests", rendering_static_data_triangle_test);
        REGISTER_PIXEL_TEST("rendering_tests", rendering_strip_quads_test);
        REGISTER_PIXEL_TEST("rendering_tests", rendering_shader_constants_test);
        REGISTER_PIXEL_TEST("rendering_tests", rendering_textured_quad_test);
        REGISTER_PIXEL_TEST("rendering_tests", rendering_render_target_test);
        REGISTER_PIXEL_TEST("rendering_tests", rendering_instanced_data_test);
        REGISTER_PIXEL_TEST("rendering_tests", rendering_triple_instancing_test);
        REGISTER_PIXEL_TEST("rendering_tests", rendering_blend_types_test);
        REGISTER_PIXEL_TEST("rendering_tests", rendering_cube_texture_test);
        REGISTER_PIXEL_TEST("rendering_tests", rendering_depth_behavior_test);
        REGISTER_PIXEL_TEST("rendering_tests", rendering_partial_data_test);
        REGISTER_PIXEL_TEST("rendering_tests", rendering_indexed_data_test);
        REGISTER_PIXEL_TEST("rendering_tests", rendering_points_and_line_strip_test);
        REGISTER_PIXEL_TEST("rendering_tests", rendering_vertex_attribute_formats_test);
        REGISTER_PIXEL_TEST("rendering_tests", rendering_multiple_render_targets_test);
        REGISTER_SCREENSHOT_TEST("rendering_tests", rendering_4x4_linear_texture_test);
    }
}
