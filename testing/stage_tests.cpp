
#include "runner.h"

namespace test {
    const char *STAGE_TEXTURE = "stage_tests/stage_texture";

    bool stage_statistics_test(const API &api) {
        {
            ui::PivotPtr pivot = api.stage->addPivot(std::nullopt, nullptr, {});
            ui::ImagePtr image = api.stage->addImage(std::nullopt, nullptr, {.texture = STAGE_TEXTURE});
            ui::Img9SlicePtr slice = api.stage->addImg9Slice(std::nullopt, nullptr, {.texture = STAGE_TEXTURE});
            ui::TextLinePtr line = api.stage->addTextLine(std::nullopt, nullptr, {});
            ui::TextBlockPtr block = api.stage->addTextBlock(std::nullopt, nullptr, {});
            api.stage->updateAndDraw(0.0f);

            EXPECT_NOT_EQ(image, nullptr);
            EXPECT_NOT_EQ(slice, nullptr);

            const ui::StageInterface::StatInfo stats = api.stage->getStatistics();
            EXPECT_EQ(stats.totalElementsCount, 5);
            EXPECT_EQ(stats.topLevelElementsCount, 5);
            EXPECT_EQ(stats.pivotsCount, 1);
            EXPECT_EQ(stats.imagesCount, 1);
            EXPECT_EQ(stats.imgCustomsCount, 0);
            EXPECT_EQ(stats.img9SlicesCount, 1);
            EXPECT_EQ(stats.textLinesCount, 1);
            EXPECT_EQ(stats.textBlocksCount, 1);
        }
        api.stage->updateAndDraw(0.0f);

        const ui::StageInterface::StatInfo after = api.stage->getStatistics();
        EXPECT_EQ(after.totalElementsCount, 0);
        EXPECT_EQ(after.topLevelElementsCount, 0);
        EXPECT_EQ(after.pivotsCount, 0);
        EXPECT_EQ(after.imagesCount, 0);
        EXPECT_EQ(after.img9SlicesCount, 0);
        EXPECT_EQ(after.textLinesCount, 0);
        EXPECT_EQ(after.textBlocksCount, 0);
        return true;
    }

    bool stage_named_element_test(const API &api) {
        ui::PivotPtr pivot = api.stage->addPivot("stage_named_pivot", nullptr, {});
        EXPECT_EQ(api.stage->getNamedElement("stage_named_pivot"), pivot);
        EXPECT_EQ(api.stage->getNamedElement("stage_unknown"), nullptr);
        return true;
    }

    bool stage_child_lifetime_test(const API &api) {
        ui::PivotPtr parent = api.stage->addPivot(std::nullopt, nullptr, {});
        std::weak_ptr<ui::StageInterface::Pivot> child = api.stage->addPivot(std::nullopt, parent, {});
        api.stage->updateAndDraw(0.0f);

        EXPECT_FALSE(child.expired());
        EXPECT_EQ(api.stage->getStatistics().totalElementsCount, 2);

        parent = nullptr;
        api.stage->updateAndDraw(0.0f);

        EXPECT_TRUE(child.expired());
        EXPECT_EQ(api.stage->getStatistics().totalElementsCount, 0);
        return true;
    }

    bool stage_anchor_test(const API &api) {
        const float w = float(api.platform->getScreenWidth()) / api.platform->getScreenScale();
        const float h = float(api.platform->getScreenHeight()) / api.platform->getScreenScale();
        const math::vector2f size = {20.0f, 10.0f};

        ui::TextBlockPtr lt = api.stage->addTextBlock(std::nullopt, nullptr, {
            .anchorH = ui::HorizontalAnchor::LEFT, .anchorV = ui::VerticalAnchor::TOP, .anchorOffset = {3.0f, 4.0f}, .size = size
        });
        ui::TextBlockPtr center = api.stage->addTextBlock(std::nullopt, nullptr, {
            .anchorH = ui::HorizontalAnchor::CENTER, .anchorV = ui::VerticalAnchor::MIDDLE, .size = size
        });
        ui::TextBlockPtr rb = api.stage->addTextBlock(std::nullopt, nullptr, {
            .anchorH = ui::HorizontalAnchor::RIGHT, .anchorV = ui::VerticalAnchor::BOTTOM, .anchorOffset = {3.0f, 4.0f}, .size = size
        });
        ui::TextBlockPtr outsideRB = api.stage->addTextBlock(std::nullopt, nullptr, {
            .anchorTarget = lt, .anchorH = ui::HorizontalAnchor::RIGHTSIDE, .anchorV = ui::VerticalAnchor::BOTTOMSIDE, .anchorOffset = {1.0f, 2.0f}, .size = size
        });
        ui::TextBlockPtr outsideLT = api.stage->addTextBlock(std::nullopt, nullptr, {
            .anchorTarget = lt, .anchorH = ui::HorizontalAnchor::LEFTSIDE, .anchorV = ui::VerticalAnchor::TOPSIDE, .anchorOffset = {1.0f, 2.0f}, .size = size
        });
        api.stage->updateAndDraw(0.0f);

        EXPECT_EQ_F(lt->getPosition().x, 3.0f);
        EXPECT_EQ_F(lt->getPosition().y, 4.0f);
        EXPECT_EQ_F(center->getPosition().x, std::floor(0.5f * (w - size.x)));
        EXPECT_EQ_F(center->getPosition().y, std::floor(0.5f * (h - size.y)));
        EXPECT_EQ_F(rb->getPosition().x, w - size.x - 3.0f);
        EXPECT_EQ_F(rb->getPosition().y, h - size.y - 4.0f);
        EXPECT_EQ_F(outsideRB->getPosition().x, 24.0f);
        EXPECT_EQ_F(outsideRB->getPosition().y, 16.0f);
        EXPECT_EQ_F(outsideLT->getPosition().x, -18.0f);
        EXPECT_EQ_F(outsideLT->getPosition().y, -8.0f);

        lt->setAnchorOffset({5.0f, 6.0f});
        api.stage->updateAndDraw(0.0f);

        EXPECT_EQ_F(lt->getPosition().x, 5.0f);
        EXPECT_EQ_F(lt->getPosition().y, 6.0f);
        EXPECT_EQ_F(outsideRB->getPosition().x, 26.0f);
        EXPECT_EQ_F(outsideRB->getPosition().y, 18.0f);
        return true;
    }

    bool stage_clear_test(const API &api) {
        {
            ui::PivotPtr pivot = api.stage->addPivot("stage_clear_pivot", nullptr, {});
            ui::TextLinePtr line = api.stage->addTextLine(std::nullopt, pivot, {});
            ui::TextBlockPtr block = api.stage->addTextBlock(std::nullopt, nullptr, {});
            api.stage->clear();
        }

        const ui::StageInterface::StatInfo stats = api.stage->getStatistics();
        EXPECT_EQ(stats.totalElementsCount, 0);
        EXPECT_EQ(stats.topLevelElementsCount, 0);
        EXPECT_EQ(stats.pivotsCount, 0);
        EXPECT_EQ(stats.textLinesCount, 0);
        EXPECT_EQ(stats.textBlocksCount, 0);
        EXPECT_EQ(api.stage->getNamedElement("stage_clear_pivot"), nullptr);
        return true;
    }
}

namespace test {
    void registerStageTests(TestRunner &runner) {
        REGISTER_UNIT_TEST(stage_statistics_test);
        REGISTER_UNIT_TEST(stage_named_element_test);
        REGISTER_UNIT_TEST(stage_child_lifetime_test);
        REGISTER_UNIT_TEST(stage_anchor_test);
        REGISTER_UNIT_TEST(stage_clear_test);
    }
}
