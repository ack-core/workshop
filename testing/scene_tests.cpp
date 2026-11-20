
#include "runner.h"

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
        std::pair<std::uint8_t, std::uint16_t> getThresholdAndMaxDiff() const override {
            return {4, 0};
        }

    private:
        core::SceneInterface::LineSetPtr _square;
    };

    class scene_custom_mesh_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<scene_custom_mesh_test>();
        }
        void init(const API &api) override {
            const char *gbufferShaderSrc = R"(
                const {
                    modelTransform : matrix4
                }
                vssrc {
                    output_position = _transform(float4(vertex_position.xyz, 1.0), _transform(const_modelTransform, frame_plmVPMatrix));
                }
                fssrc {
                    output_color[0] = float4(0.5, 0.5, 1.0, 10.0 / 255.0);
                }
            )";
            const char *screenShaderSrc = R"(
                const {
                    modelTransform : matrix4
                }
                vssrc {
                    output_position = _transform(float4(vertex_position.xyz, 1.0), _transform(const_modelTransform, frame_plmVPMatrix));
                }
                fssrc {
                    output_color[0] = float4(0.0, 1.0, 0.0, 1.0);
                }
            )";
            const foundation::InputLayout layout = foundation::InputLayout {
                .attributes = {
                    {"position", foundation::InputAttributeFormat::FLOAT3},
                }
            };
            const float vertexes[] = {
                -0.5f, -0.5f, 0.0f,
                 0.5f, -0.5f, 0.0f,
                -0.5f,  0.5f, 0.0f,
                 0.5f,  0.5f, 0.0f,
            };

            api.scene->setCameraLookAt({0.0f, 0.0f, 5.0f}, {0.0f, 0.0f, 0.0f});
            _gbufferMesh = api.scene->addCustomMesh(layout, gbufferShaderSrc, true);
            _gbufferMesh->setGeometry(foundation::RenderTopology::TRIANGLESTRIP, vertexes, 4);
            _gbufferMesh->setShaderConstants(math::transform3f::identity().translated({-1.0f, 0.0f, 0.0f}).flat);
            _screenMesh = api.scene->addCustomMesh(layout, screenShaderSrc, false);
            _screenMesh->setGeometry(foundation::RenderTopology::TRIANGLESTRIP, vertexes, 4);
            _screenMesh->setShaderConstants(math::transform3f::identity().translated({1.0f, 0.0f, 0.0f}).flat);
        }
        void updateAndDraw(const API &api) override {}
        std::pair<std::uint8_t, std::uint16_t> getThresholdAndMaxDiff() const override {
            return {4, 0};
        }

    private:
        core::SceneInterface::CustomMeshPtr _gbufferMesh;
        core::SceneInterface::CustomMeshPtr _screenMesh;
    };

    class scene_voxel_mesh_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<scene_voxel_mesh_test>();
        }
        void init(const API &api) override {
            api.scene->setCameraLookAt({40.0f, 40.0f, 40.0f}, {0.0f, 0.0f, 0.0f});
            api.resourceProvider->getOrLoadVoxelMesh("scene_tests/scene_voxel_mesh_test", [this, scene = api.scene](const std::vector<foundation::RenderDataPtr> &frames, const util::Description &desc) {
                _mesh = scene->addVoxelMesh(frames, desc);
            });
        }
        void updateAndDraw(const API &api) override {}
        std::pair<std::uint8_t, std::uint16_t> getThresholdAndMaxDiff() const override {
            return {4, 256};
        }

    private:
        core::SceneInterface::VoxelMeshPtr _mesh;
    };
}

namespace test {
    void registerSceneTests(TestRunner &runner) {
        REGISTER_UNIT_TEST(scene_statistics_test);
        REGISTER_UNIT_TEST(scene_camera_look_at_test);
        REGISTER_SCREENSHOT_TEST("scene_tests", scene_line_square_test);
        REGISTER_SCREENSHOT_TEST("scene_tests", scene_custom_mesh_test);
        REGISTER_SCREENSHOT_TEST("scene_tests", scene_voxel_mesh_test);
    }
}
