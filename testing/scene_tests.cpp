
#include "scene_tests.h"

namespace test {
    bool scene_statistics_test(const API &api) {
        {
            core::SceneInterface::ArrowsPtr arrows = api.scene->addArrows();
            core::SceneInterface::LineSetPtr lineSet0 = api.scene->addLineSet();
            core::SceneInterface::LineSetPtr lineSet1 = api.scene->addLineSet();
            core::SceneInterface::BoundingSpherePtr sphere = api.scene->addBoundingSphere({0, 0, 0}, 1.0f, {1, 1, 1, 1});
            core::SceneInterface::BoundingBoxPtr box = api.scene->addBoundingBox({0, 0, 0}, {-1, -1, -1, 1, 1, 1}, {1, 1, 1, 1});

            const core::SceneInterface::StatInfo stats = api.scene->getStatistics();
            EXPECT_EQ(stats.arrowsCount, 1);
            EXPECT_EQ(stats.lineSetsCount, 2);
            EXPECT_EQ(stats.boundingSpheresCount, 1);
            EXPECT_EQ(stats.boundingBoxesCount, 1);
            EXPECT_EQ(stats.voxelMeshesCount, 0);
            EXPECT_EQ(stats.groundMeshesCount, 0);
            EXPECT_EQ(stats.vegetationsCount, 0);
            EXPECT_EQ(stats.particleEmittersCount, 0);
        }
        api.scene->updateAndDraw(0.0f);

        const core::SceneInterface::StatInfo after = api.scene->getStatistics();
        EXPECT_EQ(after.arrowsCount, 0);
        EXPECT_EQ(after.lineSetsCount, 0);
        EXPECT_EQ(after.boundingSpheresCount, 0);
        EXPECT_EQ(after.boundingBoxesCount, 0);
        return true;
    }

    bool scene_camera_look_at_test(const API &api) {
        const math::vector3f position = {10, 10, 10};
        const math::vector3f direction = (math::vector3f(0, 0, 0) - position).normalized();
        api.scene->setCameraLookAt(position, {0, 0, 0});

        const math::vector3f cameraPosition = api.scene->getCameraPosition();
        const math::vector3f cameraDirection = api.scene->getCameraDirection();
        api.scene->setCameraLookAt({45, 45, 45}, {0, 0, 0});

        EXPECT_EQ_F(cameraPosition.x, position.x);
        EXPECT_EQ_F(cameraPosition.y, position.y);
        EXPECT_EQ_F(cameraPosition.z, position.z);
        EXPECT_EQ_F(cameraDirection.x, direction.x);
        EXPECT_EQ_F(cameraDirection.y, direction.y);
        EXPECT_EQ_F(cameraDirection.z, direction.z);
        return true;
    }
}

namespace test {
    class scene_line_square_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<scene_line_square_test>();
        }
        void init(const API &api) override {
            api.scene->setLinesDrawingEnabled(true);
            api.scene->setCameraLookAt({0.0f, 0.0f, 5.0f}, {0.0f, 0.0f, 0.0f});
            _square = api.scene->addLineSet();
            _square->fillAsClosedPolygon({{-1.0f, -1.0f, 0.0f}, {1.0f, -1.0f, 0.0f}, {1.0f, 1.0f, 0.0f}, {-1.0f, 1.0f, 0.0f}}, {1.0f, 0.0f, 0.0f, 1.0f});
        }
        void updateAndDraw(const API &api) override {}
        std::uint8_t getThreshold() const override {
            return 4;
        }

    private:
        core::SceneInterface::LineSetPtr _square;
    };
}

namespace test {
    void registerSceneTests(TestRunner &runner) {
        REGISTER_UNIT_TEST(scene_statistics_test);
        REGISTER_UNIT_TEST(scene_camera_look_at_test);
        REGISTER_SCREENSHOT_TEST(scene_line_square_test);
    }
}
