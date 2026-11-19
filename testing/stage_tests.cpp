
#include "stage_tests.h"

namespace test {
    bool stage_statistics_test(const API &api) {
        const resource::FontAtlasProviderPtr fontAtlasProvider = resource::FontAtlasProvider::instance(api.platform, api.rendering);
        const ui::StageInterfacePtr stage = ui::StageInterface::instance(api.platform, api.rendering, api.resourceProvider, fontAtlasProvider);
        const char *texture = "resource_provider_tests/resource_provider_texture_loading_test";
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
        {
            ui::PivotPtr pivot = stage->addPivot(std::nullopt, nullptr, {});
            ui::ImagePtr image = stage->addImage(std::nullopt, nullptr, {.texture = texture});
            ui::TextLinePtr textLine = stage->addTextLine(std::nullopt, nullptr, {});
            stage->addPivot(std::nullopt, pivot, {});
            stage->addImage(std::nullopt, pivot, {.texture = texture});
            stage->addImgCustom(std::nullopt, pivot, {.texture = texture, .layout = layout, .shaderSource = shaderSrc});
            stage->addImg9Slice(std::nullopt, image, {.texture = texture});
            stage->addTextLine(std::nullopt, image, {});
            stage->addTextBlock(std::nullopt, textLine, {});
            stage->updateAndDraw(0.0f);

            const ui::StageInterface::StatInfo &stats = stage->getStatistics();
            EXPECT_EQ(stats.totalElementsCount, 9);
            EXPECT_EQ(stats.topLevelElementsCount, 3);
            EXPECT_EQ(stats.pivotsCount, 2);
            EXPECT_EQ(stats.imagesCount, 2);
            EXPECT_EQ(stats.imgCustomsCount, 1);
            EXPECT_EQ(stats.img9SlicesCount, 1);
            EXPECT_EQ(stats.textLinesCount, 2);
            EXPECT_EQ(stats.textBlocksCount, 1);
        }
        stage->updateAndDraw(0.0f);

        const ui::StageInterface::StatInfo &after = stage->getStatistics();
        EXPECT_EQ(after.totalElementsCount, 0);
        EXPECT_EQ(after.topLevelElementsCount, 0);
        EXPECT_EQ(after.pivotsCount, 0);
        EXPECT_EQ(after.imagesCount, 0);
        EXPECT_EQ(after.imgCustomsCount, 0);
        EXPECT_EQ(after.img9SlicesCount, 0);
        EXPECT_EQ(after.textLinesCount, 0);
        EXPECT_EQ(after.textBlocksCount, 0);
        return true;
    }
}

namespace test {
    void registerStageTests(TestRunner &runner) {
        REGISTER_UNIT_TEST(stage_statistics_test);
    }
}
