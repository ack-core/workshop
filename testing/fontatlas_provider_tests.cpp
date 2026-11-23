
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
}

namespace test {
    class fontatlas_provider_blur_precache_test : public StepTestInterface {
    public:
        static auto make() -> std::unique_ptr<StepTestInterface> {
            return std::make_unique<fontatlas_provider_blur_precache_test>();
        }
        void init(const API &api) override {
            api.fontAtlasProvider->prepareFontAtlas(TEXT, FONT_SIZE, 2, [this, provider = api.fontAtlasProvider]() {
                _blurredTexture = provider->getTextFontAtlas(TEXT, FONT_SIZE, 2, _blurred);
                _unblurredTexture = provider->getTextFontAtlas(TEXT, FONT_SIZE, 0, _unblurred);
            });
        }
        bool testResult(const API &api) override {
            EXPECT_NOT_EQ(_blurredTexture, nullptr);
            EXPECT_NOT_EQ(_unblurredTexture, nullptr);
            EXPECT_EQ(_blurred.size(), 4);
            EXPECT_EQ(_unblurred.size(), 4);
            EXPECT_LT(_unblurred[0].pxSize.x, _blurred[0].pxSize.x);
            return true;
        }

    private:
        static constexpr const char *TEXT = "Blur";
        static constexpr std::uint8_t FONT_SIZE = 24;

        foundation::RenderTexturePtr _blurredTexture, _unblurredTexture;
        std::vector<resource::FontCharInfo> _blurred, _unblurred;
    };
}

namespace test {
    class fontatlas_provider_atlas_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<fontatlas_provider_atlas_test>();
        }
        void init(const API &api) override {
            // Fullscreen quad sampling the font atlas texture
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
                    float r = _tex2d(0, input_uv).r;
                    output_color[0] = float4(r, r, r, 1.0);
                }
            )";
            const foundation::InputLayout layout = foundation::InputLayout {
                .repeat = 4
            };
            _shader = api.rendering->createShader(shaderSrc, layout);
            api.fontAtlasProvider->prepareFontAtlas(TEXT, FONT_SIZE, 0, {});
        }
        void updateAndDraw(const API &api) override {
            std::vector<resource::FontCharInfo> chars;
            const foundation::RenderTexturePtr &texture = api.fontAtlasProvider->getTextFontAtlas(TEXT, FONT_SIZE, 0, chars);
            api.rendering->forTarget(nullptr, nullptr, math::color{ 0.0f, 0.0f, 0.0f, 1.0f }, [&](foundation::RenderingInterface &rendering) {
                rendering.applyShader(_shader, foundation::RenderTopology::TRIANGLESTRIP, foundation::BlendType::DISABLED, foundation::DepthBehavior::DISABLED);
                rendering.applyTextures({ { texture, foundation::SamplerType::NEAREST } });
                rendering.draw(1);
            });
        }
        std::pair<std::uint8_t, std::uint16_t> getThresholdAndMaxDiff() const override {
            return {4, 0};
        }

    private:
        static constexpr const char *TEXT = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
        static constexpr std::uint8_t FONT_SIZE = 32;

        foundation::RenderShaderPtr _shader;
    };
}

namespace test {
    void registerFontAtlasProviderTests(TestRunner& runner) {
        REGISTER_UNIT_TEST(fontatlas_provider_non_uniquity_test);
        REGISTER_STEP_TEST(fontatlas_provider_blur_precache_test);
        REGISTER_SCREENSHOT_TEST("fontatlas_provider_tests", fontatlas_provider_atlas_test);
    }
}
