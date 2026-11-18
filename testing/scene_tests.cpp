
#include "scene_tests.h"

namespace test {
    bool scene_statistics_test(const API &api) {
        core::SceneInterfacePtr scene = core::SceneInterface::instance(api.platform, api.rendering);
        {
            core::SceneInterface::ArrowsPtr arrows = scene->addArrows();
            core::SceneInterface::LineSetPtr lineSet0 = scene->addLineSet();
            core::SceneInterface::LineSetPtr lineSet1 = scene->addLineSet();
            core::SceneInterface::BoundingSpherePtr sphere = scene->addBoundingSphere({0, 0, 0}, 1.0f, {1, 1, 1, 1});
            core::SceneInterface::BoundingBoxPtr box = scene->addBoundingBox({0, 0, 0}, {-1, -1, -1, 1, 1, 1}, {1, 1, 1, 1});
            scene->updateAndDraw(0.0f);

            const core::SceneInterface::StatInfo &stats = scene->getStatistics();
            EXPECT_EQ(stats.arrowsCount, 1);
            EXPECT_EQ(stats.lineSetsCount, 2);
            EXPECT_EQ(stats.boundingSpheresCount, 1);
            EXPECT_EQ(stats.boundingBoxesCount, 1);
            EXPECT_EQ(stats.voxelMeshesCount, 0);
            EXPECT_EQ(stats.groundMeshesCount, 0);
            EXPECT_EQ(stats.vegetationsCount, 0);
            EXPECT_EQ(stats.particleEmittersCount, 0);
        }
        scene->updateAndDraw(0.0f);

        const core::SceneInterface::StatInfo &after = scene->getStatistics();
        EXPECT_EQ(after.arrowsCount, 0);
        EXPECT_EQ(after.lineSetsCount, 0);
        EXPECT_EQ(after.boundingSpheresCount, 0);
        EXPECT_EQ(after.boundingBoxesCount, 0);
        return true;
    }
}

namespace test {
    void registerSceneTests(TestRunner &runner) {
        REGISTER_UNIT_TEST(scene_statistics_test);
    }
}
