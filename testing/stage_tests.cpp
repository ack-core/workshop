
#include "runner.h"

namespace {
    math::vector2f stage_send_pointer(const test::API &api, foundation::PlatformPointerEventArgs::EventType type, float x, float y) {
        const float scale = api.platform->getScreenScale();
        foundation::PlatformPointerEventArgs args {};
        args.type = type;
        args.pointerID = 0;
        args.coordinateX = std::floor(x * scale + 0.5f);
        args.coordinateY = std::floor(y * scale + 0.5f);
        api.platform->sendPointerEvent(args);
        return {args.coordinateX / scale, args.coordinateY / scale};
    }
}

namespace test {
    bool stage_statistics_test(const API &api) {
        api.platform->setSuppressOutput(true, false);
        const char *STAGE_TEXTURE = "stage_tests/stage_texture";
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

    bool stage_interactor_action_handler_test(const API &api) {
        api.platform->setSuppressOutput(true, false);
        using EventType = foundation::PlatformPointerEventArgs::EventType;
        std::string calls, states;
        math::vector2f point = {0.0f, 0.0f};

        ui::ImagePtr image = api.stage->addImage(std::nullopt, nullptr, {.anchorOffset = {10.0f, 20.0f}, .texture = "stage_tests/stage_texture"});
        image->setActionHandler([&](ui::Action action, float x, float y) {
            calls += action == ui::Action::PRESS ? "P" : action == ui::Action::MOVE ? "M" : "R";
            point = {x, y};
        });
        image->setStateHandler([&](bool locked) {
            states += locked ? "L" : "U";
        });
        api.stage->updateAndDraw(0.0f);

        const float cx = 10.0f + 0.5f * image->getSize().x;
        const float cy = 20.0f + 0.5f * image->getSize().y;

        const math::vector2f press = stage_send_pointer(api, EventType::START, cx, cy);
        EXPECT_EQ(calls, "P");
        EXPECT_EQ_F(point.x, press.x);
        EXPECT_EQ_F(point.y, press.y);
        const math::vector2f move = stage_send_pointer(api, EventType::MOVE, 2.0f, 3.0f);
        EXPECT_EQ(calls, "PM");
        EXPECT_EQ_F(point.x, move.x);
        EXPECT_EQ_F(point.y, move.y);
        const math::vector2f release = stage_send_pointer(api, EventType::FINISH, 4.0f, 5.0f);
        EXPECT_EQ(calls, "PMR");
        EXPECT_EQ_F(point.x, release.x);
        EXPECT_EQ_F(point.y, release.y);

        calls.clear();
        stage_send_pointer(api, EventType::START, 9.0f, cy);
        stage_send_pointer(api, EventType::MOVE, cx, cy);
        stage_send_pointer(api, EventType::FINISH, cx, cy);
        EXPECT_TRUE(calls.empty());

        image->setLocked(true);
        image->setLocked(true);
        EXPECT_EQ(states, "L");
        stage_send_pointer(api, EventType::START, cx, cy);
        stage_send_pointer(api, EventType::MOVE, cx, cy);
        stage_send_pointer(api, EventType::FINISH, cx, cy);
        EXPECT_TRUE(calls.empty());

        image->setLocked(false);
        EXPECT_EQ(states, "LU");
        stage_send_pointer(api, EventType::START, cx, cy);
        stage_send_pointer(api, EventType::FINISH, cx, cy);
        EXPECT_EQ(calls, "PR");
        return true;
    }

    bool stage_interactor_active_area_test(const API &api) {
        api.platform->setSuppressOutput(true, false);
        using EventType = foundation::PlatformPointerEventArgs::EventType;
        const float u = 1.0f / api.platform->getScreenScale();
        std::string calls;

        ui::Img9SlicePtr slice = api.stage->addImg9Slice(std::nullopt, nullptr, {
            .anchorOffset = {10.0f * u, 20.0f * u}, .size = {40.0f * u, 20.0f * u}, .texture = "stage_tests/stage_texture", .activeAreaOffset = 5.0f * u, .activeAreaRadius = 3.0f * u
        });
        slice->setActionHandler([&](ui::Action action, float x, float y) {
            calls += action == ui::Action::PRESS ? "P" : "R";
        });
        api.stage->updateAndDraw(0.0f);

        const auto tap = [&](float x, float y) {
            calls.clear();
            stage_send_pointer(api, EventType::START, x * u, y * u);
            stage_send_pointer(api, EventType::FINISH, x * u, y * u);
            return calls;
        };

        EXPECT_EQ(tap(30.0f, 30.0f), "PR");
        EXPECT_EQ(tap(13.0f, 30.0f), "PR");
        EXPECT_EQ(tap(11.0f, 30.0f), "");
        EXPECT_EQ(tap(47.0f, 30.0f), "PR");
        EXPECT_EQ(tap(49.0f, 30.0f), "");
        EXPECT_EQ(tap(30.0f, 37.0f), "PR");
        EXPECT_EQ(tap(30.0f, 39.0f), "");
        EXPECT_EQ(tap(14.0f, 24.0f), "PR");
        EXPECT_EQ(tap(12.0f, 22.0f), "");
        EXPECT_EQ(tap(46.0f, 36.0f), "PR");
        EXPECT_EQ(tap(48.0f, 38.0f), "");
        return true;
    }
}

namespace test {
    class stage_pivot_screen_world_position_test : public StepTestInterface {
    public:
        static auto make() -> std::unique_ptr<StepTestInterface> {
            return std::make_unique<stage_pivot_screen_world_position_test>();
        }
        void init(const API &api) override {
            api.scene->setCameraLookAt({45.0f, 45.0f, 45.0f}, {0.0f, 0.0f, 0.0f});
            _screen = api.stage->addPivot(std::nullopt, nullptr, {.anchorOffset = {7.0f, 8.0f}});
            _screen->setWorldPosition(WORLD_POSITION);
            _screen->setScreenPosition({30.0f, 40.0f});
            _world = api.stage->addPivot(std::nullopt, nullptr, {});
            _world->setScreenPosition({30.0f, 40.0f});
            _world->setWorldPosition(WORLD_POSITION);
            _screenChild = api.stage->addPivot(std::nullopt, _screen, {.anchorOffset = {1.0f, 2.0f}});
            _worldChild = api.stage->addPivot(std::nullopt, _world, {.anchorOffset = {1.0f, 2.0f}});
        }
        bool testResult(const API &api) override {
            const math::vector2f expected = api.scene->getScreenCoordinates(WORLD_POSITION) / api.platform->getScreenScale();

            EXPECT_EQ_F(_screen->getPosition().x, 30.0f);
            EXPECT_EQ_F(_screen->getPosition().y, 40.0f);
            EXPECT_EQ_F(_screenChild->getPosition().x, 31.0f);
            EXPECT_EQ_F(_screenChild->getPosition().y, 42.0f);
            EXPECT_LT(std::fabs(_world->getPosition().x - expected.x), 0.01f);
            EXPECT_LT(std::fabs(_world->getPosition().y - expected.y), 0.01f);
            EXPECT_LT(std::fabs(_worldChild->getPosition().x - expected.x - 1.0f), 0.01f);
            EXPECT_LT(std::fabs(_worldChild->getPosition().y - expected.y - 2.0f), 0.01f);
            return true;
        }

    private:
        const math::vector3f WORLD_POSITION = {3.0f, 2.0f, -1.0f};
        ui::PivotPtr _screen, _world, _screenChild, _worldChild;
    };
}

namespace test {
    template<bool EMPTY> class stage_text_line_size_test_base : public StepTestInterface {
    public:
        static auto make() -> std::unique_ptr<StepTestInterface> {
            return std::make_unique<stage_text_line_size_test_base<EMPTY>>();
        }
        void init(const API &api) override {
            _line = api.stage->addTextLine(std::nullopt, nullptr, {.anchorOffset = {3.0f, 4.0f}, .fontSize = FONT_SIZE});
            _next = api.stage->addTextBlock(std::nullopt, nullptr, {.anchorTarget = _line, .anchorH = ui::HorizontalAnchor::RIGHTSIDE});
            api.fontAtlasProvider->prepareFontAtlas(TEXT, FONT_SIZE, 0, [this]() {
                _line->setText(TEXT);
            });
        }
        bool testResult(const API &api) override {
            const math::vector2f expected = api.fontAtlasProvider->getTextWidth(TEXT, FONT_SIZE);
            EXPECT_EQ(expected.x > 0.0f, !EMPTY);
            EXPECT_EQ_F(_line->getSize().x, expected.x);
            EXPECT_EQ_F(_line->getSize().y, float(FONT_SIZE));
            EXPECT_EQ_F(_next->getPosition().x, 3.0f + expected.x);
            EXPECT_TRUE(EMPTY || _line->getTexture() != nullptr);
            return true;
        }

    private:
        static constexpr const char *TEXT = EMPTY ? "" : "Size";
        static constexpr std::uint8_t FONT_SIZE = 16;

        ui::TextLinePtr _line;
        ui::TextBlockPtr _next;
    };

    using stage_text_line_empty_size_test = stage_text_line_size_test_base<true>;
    using stage_text_line_size_test = stage_text_line_size_test_base<false>;
}

namespace test {
    class stage_img9slice_rendering_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<stage_img9slice_rendering_test>();
        }
        void init(const API &api) override {
            api.resourceProvider->getOrLoadTexture(TEXTURE, [this, stage = api.stage](const foundation::RenderTexturePtr &) {
                _grown = stage->addImg9Slice(std::nullopt, nullptr, {.anchorOffset = {16.0f, 16.0f}, .size = {64.0f, 64.0f}, .texture = TEXTURE, .sliceArgs = SLICE_ARGS});
                _grown->setSize({160.0f, 88.0f});
                _clamped = stage->addImg9Slice(std::nullopt, nullptr, {.anchorOffset = {200.0f, 16.0f}, .texture = TEXTURE, .sliceArgs = SLICE_ARGS});
                _clamped->setSize({8.0f, 8.0f});
                _hollow = stage->addImg9Slice(std::nullopt, nullptr, {.anchorOffset = {16.0f, 128.0f}, .size = {96.0f, 96.0f}, .texture = TEXTURE, .sliceArgs = SLICE_ARGS, .fill = false});
            });
        }
        void updateAndDraw(const API &api) override {}
        std::uint8_t getThreshold() const override {
            return 4;
        }

        bool testResult(const API &api, std::size_t outOfThresholdPixelCount) override {
            return outOfThresholdPixelCount <= 0;
        }

    private:
        static constexpr const char *TEXTURE = "stage_tests/stage_img9slice_rendering_test";
        const math::vector3f SLICE_ARGS = {12.0f, 12.0f, 16.0f};
        ui::Img9SlicePtr _grown, _clamped, _hollow;
    };
}

namespace test {
    void registerStageTests(TestRunner &runner) {
        REGISTER_UNIT_TEST(stage_statistics_test);
        REGISTER_UNIT_TEST(stage_named_element_test);
        REGISTER_UNIT_TEST(stage_child_lifetime_test);
        REGISTER_UNIT_TEST(stage_anchor_test);
        REGISTER_UNIT_TEST(stage_clear_test);
        REGISTER_UNIT_TEST(stage_interactor_action_handler_test);
        REGISTER_UNIT_TEST(stage_interactor_active_area_test);
        REGISTER_STEP_TEST(stage_pivot_screen_world_position_test);
        REGISTER_STEP_TEST(stage_text_line_empty_size_test);
        REGISTER_STEP_TEST(stage_text_line_size_test);
        REGISTER_SCREENSHOT_TEST("stage_tests", stage_img9slice_rendering_test);
    }
}
