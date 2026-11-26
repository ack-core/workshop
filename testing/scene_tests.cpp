
#include "runner.h"
#include "foundation/layouts.h"

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

    bool scene_screen_world_projection_test(const API &api) {
        const float width = api.platform->getScreenWidth();
        const float height = api.platform->getScreenHeight();
        api.scene->setCameraLookAt({10, 0, 0}, {0, 0, 0});

        const math::vector3f right = api.scene->getCameraRight();
        const math::vector2f center = api.scene->getScreenCoordinates({0, 0, 0});
        const math::vector2f behind = api.scene->getScreenCoordinates({20, 0, 0});
        const math::vector2f shifted = api.scene->getScreenCoordinates(math::vector3f(0, 1, 0) + right * 2.0f);

        math::vector3f camPosition;
        const math::vector3f centerDir = api.scene->getWorldDirection({width * 0.5f, height * 0.5f}, &camPosition);
        const math::vector3f shiftedDir = api.scene->getWorldDirection(shifted);
        const math::vector3f expectedDir = (math::vector3f(0, 1, 0) + right * 2.0f - camPosition).normalized();
        api.scene->setCameraLookAt({45, 45, 45}, {0, 0, 0});

        EXPECT_EQ_F(right.x, 0.0f);
        EXPECT_EQ_F(right.y, 0.0f);
        EXPECT_EQ_F(right.z, -1.0f);
        EXPECT_LT(std::fabs(center.x - width * 0.5f), 0.01f);
        EXPECT_LT(std::fabs(center.y - height * 0.5f), 0.01f);
        EXPECT_EQ(behind.x, std::numeric_limits<float>::min());
        EXPECT_EQ(behind.y, std::numeric_limits<float>::min());
        EXPECT_LT(width * 0.5f, shifted.x);
        EXPECT_LT(shifted.y, height * 0.5f);
        EXPECT_EQ_F(camPosition.x, 10.0f);
        EXPECT_EQ_F(camPosition.y, 0.0f);
        EXPECT_EQ_F(camPosition.z, 0.0f);
        EXPECT_LT(std::fabs(centerDir.x + 1.0f), 1.0e-4f);
        EXPECT_LT(std::fabs(centerDir.y), 1.0e-4f);
        EXPECT_LT(std::fabs(centerDir.z), 1.0e-4f);
        EXPECT_LT(std::fabs(shiftedDir.x - expectedDir.x), 1.0e-4f);
        EXPECT_LT(std::fabs(shiftedDir.y - expectedDir.y), 1.0e-4f);
        EXPECT_LT(std::fabs(shiftedDir.z - expectedDir.z), 1.0e-4f);
        return true;
    }

    bool scene_voxel_mesh_frames_test(const API &api) {
        struct Voxel {
            std::int16_t positionX, positionY, positionZ;
            std::uint8_t colorIndex, mask, scaleX, scaleY, scaleZ, reserved;
        };
        const Voxel voxel = {0, 0, 0, 10, 0x3f, 1, 1, 1, 0};
        const util::Description desc = util::Description::parse("offset : vector3f = 1 2 3\nname : string = \"box\"");
        core::SceneInterface::VoxelMeshPtr mesh = api.scene->addVoxelMesh({
            api.rendering->createData(layouts::VTXMVOX, &voxel, 1),
            api.rendering->createData(layouts::VTXMVOX, &voxel, 1),
            api.rendering->createData(layouts::VTXMVOX, &voxel, 1),
        }, desc);

        EXPECT_EQ(mesh->getFrameCount(), 3u);
        EXPECT_EQ(*mesh->getDescription().getString("name"), "box");
        EXPECT_EQ_F(mesh->getCenterOffset().x, 1.0f);
        EXPECT_EQ_F(mesh->getCenterOffset().y, 2.0f);
        EXPECT_EQ_F(mesh->getCenterOffset().z, 3.0f);

        mesh->setCenterOffset({4.0f, -5.0f, 6.0f});
        EXPECT_EQ_F(mesh->getCenterOffset().x, 4.0f);
        EXPECT_EQ_F(mesh->getCenterOffset().y, -5.0f);
        EXPECT_EQ_F(mesh->getCenterOffset().z, 6.0f);

        mesh->resetOffset();
        EXPECT_EQ_F(mesh->getCenterOffset().x, 1.0f);
        EXPECT_EQ_F(mesh->getCenterOffset().y, 2.0f);
        EXPECT_EQ_F(mesh->getCenterOffset().z, 3.0f);
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

        bool testResult(const API &api, std::size_t outOfThresholdPixelCount) override {
            return outOfThresholdPixelCount <= 0;
        }

    private:
        core::SceneInterface::LineSetPtr _square;
    };
}

namespace test {
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
        std::uint8_t getThreshold() const override {
            return 4;
        }

        bool testResult(const API &api, std::size_t outOfThresholdPixelCount) override {
            return outOfThresholdPixelCount <= 0;
        }

    private:
        core::SceneInterface::CustomMeshPtr _gbufferMesh;
        core::SceneInterface::CustomMeshPtr _screenMesh;
    };
}

namespace test {
    class scene_voxel_mesh_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<scene_voxel_mesh_test>();
        }
        void init(const API &api) override {
            api.platform->setSuppressOutput(true, false);
            api.scene->setCameraLookAt({40.0f, 40.0f, 40.0f}, {0.0f, 0.0f, 0.0f});
            api.resourceProvider->getOrLoadVoxelMesh("scene_tests/scene_voxel_mesh_test", [this, scene = api.scene](const std::vector<foundation::RenderDataPtr> &frames, const util::Description &desc) {
                _mesh = scene->addVoxelMesh(frames, desc);
            });
        }
        void updateAndDraw(const API &api) override {}
        std::uint8_t getThreshold() const override {
            return 16;
        }

        bool testResult(const API &api, std::size_t outOfThresholdPixelCount) override {
            return outOfThresholdPixelCount <= 1024;
        }

    private:
        core::SceneInterface::VoxelMeshPtr _mesh;
    };
}

namespace test {
    class scene_voxel_mesh_scale_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<scene_voxel_mesh_scale_test>();
        }
        void init(const API &api) override {
            struct Voxel {
                std::int16_t positionX, positionY, positionZ;
                std::uint8_t colorIndex, mask, scaleX, scaleY, scaleZ, reserved;
            };
            const Voxel voxels[] = {
                {0, 0, 0, 10, 0x3f, 1, 1, 1, 0},
                {2, 0, 0, 10, 0x3f, 3, 1, 1, 0},
                {0, 0, 2, 10, 0x3f, 1, 3, 1, 0},
                {-4, 0, -4, 10, 0x3f, 2, 2, 2, 0},
            };
            api.scene->setCameraLookAt({10.0f, 10.0f, 10.0f}, {0.0f, 0.0f, 0.0f});
            _mesh = api.scene->addVoxelMesh({api.rendering->createData(layouts::VTXMVOX, voxels, 4)}, {});
            _mesh->setTransform(math::transform3f({0.0f, 1.0f, 0.0f}, 0.6f).scaled({1.2f, 0.8f, 1.0f}).translated({0.0f, 1.0f, -1.0f}));
        }
        void updateAndDraw(const API &api) override {}
        std::uint8_t getThreshold() const override {
            return 16;
        }

        bool testResult(const API &api, std::size_t outOfThresholdPixelCount) override {
            return outOfThresholdPixelCount <= 128;
        }

    private:
        core::SceneInterface::VoxelMeshPtr _mesh;
    };
}

namespace test {
    class scene_voxel_mesh_set_frame_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<scene_voxel_mesh_set_frame_test>();
        }
        void init(const API &api) override {
            struct Voxel {
                std::int16_t positionX, positionY, positionZ;
                std::uint8_t colorIndex, mask, scaleX, scaleY, scaleZ, reserved;
            };
            const Voxel frame0[] = {
                {0, 0, 0, 10, 0x3f, 1, 1, 1, 0},
            };
            const Voxel frame1[] = {
                {0, 0, 0, 40, 0x3f, 1, 1, 1, 0},
                {0, 1, 0, 40, 0x3f, 1, 1, 1, 0},
            };
            api.scene->setCameraLookAt({10.0f, 10.0f, 10.0f}, {0.0f, 0.0f, 0.0f});
            for (int i = 0; i < 3; i++) {
                _meshes[i] = api.scene->addVoxelMesh({
                    api.rendering->createData(layouts::VTXMVOX, frame0, 1),
                    api.rendering->createData(layouts::VTXMVOX, frame1, 2),
                }, {});
                _meshes[i]->setPosition({float(i * 3 - 3), 0.0f, float(3 - i * 3)});
            }
            _meshes[1]->setFrame(1);
            _meshes[2]->setFrame(5);
        }
        void updateAndDraw(const API &api) override {}
        std::uint8_t getThreshold() const override {
            return 16;
        }

        bool testResult(const API &api, std::size_t outOfThresholdPixelCount) override {
            return outOfThresholdPixelCount <= 128;
        }

    private:
        core::SceneInterface::VoxelMeshPtr _meshes[3];
    };
}

namespace test {
    class scene_voxel_mesh_wireframe_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<scene_voxel_mesh_wireframe_test>();
        }
        ~scene_voxel_mesh_wireframe_test() override {
            _scene->setWireframeEnabled(false);
        }
        void init(const API &api) override {
            struct Voxel {
                std::int16_t positionX, positionY, positionZ;
                std::uint8_t colorIndex, mask, scaleX, scaleY, scaleZ, reserved;
            };
            const Voxel voxels[] = {
                {0, 0, 0, 10, 0x3f, 1, 1, 1, 0},
                {-4, 0, -4, 40, 0x3f, 2, 2, 2, 0},
            };
            _scene = api.scene;
            _scene->setWireframeEnabled(true);
            _scene->setCameraLookAt({10.0f, 10.0f, 10.0f}, {0.0f, 0.0f, 0.0f});
            _mesh = _scene->addVoxelMesh({api.rendering->createData(layouts::VTXMVOX, voxels, 2)}, {});
        }
        void updateAndDraw(const API &api) override {}
        std::uint8_t getThreshold() const override {
            return 16;
        }

        bool testResult(const API &api, std::size_t outOfThresholdPixelCount) override {
            return outOfThresholdPixelCount <= 128;
        }

    private:
        core::SceneInterfacePtr _scene;
        core::SceneInterface::VoxelMeshPtr _mesh;
    };
}

namespace test {
    class scene_bounding_shapes_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<scene_bounding_shapes_test>();
        }
        void init(const API &api) override {
            api.scene->setLinesDrawingEnabled(true);
            api.scene->setCameraLookAt({0.0f, 0.0f, 6.0f}, {0.0f, 0.0f, 0.0f});

            _sphere = api.scene->addBoundingSphere({0.0f, 0.0f, 0.0f}, 0.2f, {1.0f, 1.0f, 1.0f, 1.0f});
            _sphere->setRadius(1.0f);
            _sphere->setColor({1.0f, 0.0f, 0.0f, 1.0f});
            _sphere->setPosition({-1.5f, 0.0f, 0.0f});

            _box = api.scene->addBoundingBox({0.0f, 0.0f, 0.0f}, {-0.1f, -0.1f, -0.1f, 0.1f, 0.1f, 0.1f}, {1.0f, 1.0f, 1.0f, 1.0f});
            _box->setBBox({-0.5f, -1.0f, -0.5f, 0.5f, 1.0f, 0.5f});
            _box->setColor({0.0f, 1.0f, 0.0f, 1.0f});
            _box->setPosition({1.5f, 0.0f, 0.0f});

            _hiddenSphere = api.scene->addBoundingSphere({0.0f, 0.0f, 0.0f}, 0.5f, {0.0f, 0.0f, 1.0f, 1.0f});
            _hiddenSphere->setEnabled(false);
            _hiddenBox = api.scene->addBoundingBox({0.0f, 0.0f, 0.0f}, {-0.3f, -0.3f, -0.3f, 0.3f, 0.3f, 0.3f}, {0.0f, 0.0f, 1.0f, 1.0f});
            _hiddenBox->setEnabled(false);
        }
        void updateAndDraw(const API &api) override {}
        std::uint8_t getThreshold() const override {
            return 4;
        }

        bool testResult(const API &api, std::size_t outOfThresholdPixelCount) override {
            return outOfThresholdPixelCount <= 128;
        }

    private:
        core::SceneInterface::BoundingSpherePtr _sphere;
        core::SceneInterface::BoundingBoxPtr _box;
        core::SceneInterface::BoundingSpherePtr _hiddenSphere;
        core::SceneInterface::BoundingBoxPtr _hiddenBox;
    };
}

namespace test {
    class scene_environment_fog_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<scene_environment_fog_test>();
        }
        ~scene_environment_fog_test() override {
            _scene->setEnvironment({0.3f, 0.3f, 0.3f, 1.0f}, {0.5f, 1.0f, 0.3f}, {1.0f, 1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f, 1.0f}, {9000.0f, 10000.0f});
        }
        void init(const API &api) override {
            api.platform->setSuppressOutput(true, false);
            _scene = api.scene;
            _scene->setCameraLookAt({40.0f, 40.0f, 40.0f}, {0.0f, 0.0f, 0.0f});
            _scene->setEnvironment({0.3f, 0.3f, 0.3f, 1.0f}, {1.0f, 1.0f, 0.0f}, {1.0f, 0.9f, 0.8f, 1.0f}, {0.0f, 0.0f, 0.0f}, {0.4f, 0.6f, 0.8f, 1.0f}, {10.0f, 50.0f});
            _addMesh(api, "scene_tests/scene_environment_fog_test_voxplane", {-32.0f, -1.0f, -32.0f});
            _addMesh(api, "scene_tests/scene_voxel_mesh_test", {-20.0f, 0.0f, -20.0f});
            _addMesh(api, "scene_tests/scene_environment_fog_test_debris", {5.0f, 0.0f, 5.0f});
            _addMesh(api, "scene_tests/scene_environment_fog_test_knight", {0.0f, 0.0f, 0.0f});
        }
        void updateAndDraw(const API &api) override {}
        std::uint8_t getThreshold() const override {
            return 16;
        }

        bool testResult(const API &api, std::size_t outOfThresholdPixelCount) override {
            return outOfThresholdPixelCount <= 1024;
        }

    private:
        void _addMesh(const API &api, const char *path, const math::vector3f &position) {
            api.resourceProvider->getOrLoadVoxelMesh(path, [this, position](const std::vector<foundation::RenderDataPtr> &frames, const util::Description &desc) {
                _meshes.emplace_back(_scene->addVoxelMesh(frames, desc))->setPosition(position);
            });
        }

        core::SceneInterfacePtr _scene;
        std::vector<core::SceneInterface::VoxelMeshPtr> _meshes;
    };
}

namespace test {
    class scene_arrows_and_line_set_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<scene_arrows_and_line_set_test>();
        }
        void init(const API &api) override {
            api.scene->setLinesDrawingEnabled(true);
            api.scene->setCameraLookAt({0.0f, 0.0f, 6.0f}, {0.0f, 0.0f, 0.0f});

            _arrows = api.scene->addArrows();
            _arrows->setArrow(0, {0.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f});
            _arrows->setArrow(1, {0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f});
            _arrows->setPosition({-1.97f, 0.53f, 0.0f});

            _lines = api.scene->addLineSet();
            _lines->setLine(0, {-0.5f, -0.3f, 0.0f}, {0.5f, 0.3f, 0.0f}, {1.0f, 1.0f, 0.0f, 1.0f});
            _lines->setLine(1, {-0.3f, 0.5f, 0.0f}, {0.27f, -0.5f, 0.0f}, {0.0f, 1.0f, 1.0f, 1.0f});
            _lines->setPosition({0.03f, -0.97f, 0.0f});

            _circle = api.scene->addLineSet();
            _circle->fillAsCircle(32, 0.5f, {1.0f, 0.0f, 1.0f, 1.0f});
            _circle->setTransform(math::transform3f::identity().scaled({2.0f, 1.0f, 1.0f}).translated({1.5f, 1.0f, 0.0f}));

            _hiddenArrows = api.scene->addArrows();
            _hiddenArrows->setArrow(0, {0.0f, -2.0f, 0.0f}, {0.0f, 2.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
            _hiddenArrows->setEnabled(false);

            _clearedArrows = api.scene->addArrows();
            _clearedArrows->setArrow(0, {-2.0f, -2.0f, 0.0f}, {2.0f, 2.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
            _clearedArrows->clear();

            _hiddenLines = api.scene->addLineSet();
            _hiddenLines->fillAsCircle(16, 1.5f, {1.0f, 1.0f, 1.0f, 1.0f});
            _hiddenLines->setEnabled(false);

            _clearedLines = api.scene->addLineSet();
            _clearedLines->setLine(0, {-2.0f, 2.0f, 0.0f}, {2.0f, -2.0f, 0.0f}, {1.0f, 1.0f, 1.0f, 1.0f});
            _clearedLines->clear();
        }
        void updateAndDraw(const API &api) override {}
        std::uint8_t getThreshold() const override {
            return 4;
        }

        bool testResult(const API &api, std::size_t outOfThresholdPixelCount) override {
            return outOfThresholdPixelCount <= 16;
        }

    private:
        core::SceneInterface::ArrowsPtr _arrows;
        core::SceneInterface::LineSetPtr _lines;
        core::SceneInterface::LineSetPtr _circle;
        core::SceneInterface::ArrowsPtr _hiddenArrows;
        core::SceneInterface::ArrowsPtr _clearedArrows;
        core::SceneInterface::LineSetPtr _hiddenLines;
        core::SceneInterface::LineSetPtr _clearedLines;
    };
}

namespace test {
    class scene_ground_mesh_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<scene_ground_mesh_test>();
        }
        void init(const API &api) override {
            struct Vertex {
                math::vector3f position, normal;
                math::vector2f uv;
            };
            const Vertex vertexes[] = {
                {{-1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
                {{-1.0f, 0.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
                {{ 1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                {{ 1.0f, 0.0f, -1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
                {{-1.0f, 0.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
                {{ 1.0f, 0.0f,  1.0f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
            };
            const std::uint8_t indexes[] = {
                10, 10, 40, 40,
                10, 10, 40, 40,
                57, 57, 120, 120,
                57, 57, 120, 120,
            };
            const foundation::RenderDataPtr mesh = api.rendering->createData(layouts::VTXNRMUV, vertexes, 6);
            const foundation::RenderTexturePtr texture = api.rendering->createTexture(foundation::RenderTextureFormat::R8UN, 4, 4, {indexes});

            api.scene->setCameraLookAt({0.0f, 8.0f, 4.0f}, {0.0f, 0.0f, 0.0f});
            _positioned = api.scene->addGroundMesh(mesh, texture);
            _positioned->setPosition({-2.0f, 0.0f, 0.0f});
            _transformed = api.scene->addGroundMesh(mesh, texture);
            _transformed->setTransform(math::transform3f({0.0f, 1.0f, 0.0f}, 0.6f).translated({2.0f, 0.0f, 0.0f}));
            _hidden = api.scene->addGroundMesh(mesh, texture);
            _hidden->setPosition({0.0f, 0.0f, 2.0f});
            _hidden->setEnabled(false);
        }
        void updateAndDraw(const API &api) override {}
        std::uint8_t getThreshold() const override {
            return 16;
        }

        bool testResult(const API &api, std::size_t outOfThresholdPixelCount) override {
            return outOfThresholdPixelCount <= 128;
        }

    private:
        core::SceneInterface::GroundMeshPtr _positioned;
        core::SceneInterface::GroundMeshPtr _transformed;
        core::SceneInterface::GroundMeshPtr _hidden;
    };

    class scene_textured_vegetation_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<scene_textured_vegetation_test>();
        }
        void init(const API &api) override {
            const std::uint8_t cells[] = {
                1, 1, 0, 0,   255, 255, 0, 0,   1, 1, 0, 0,
                255, 255, 0, 0,   1, 2, 0, 0,   255, 255, 0, 0,
                1, 1, 0, 0,   255, 255, 0, 0,   1, 2, 0, 0,
            };
            const std::uint8_t indexes[] = {
                10, 0,
                40, 120,
            };
            ByteDataPtr map = std::make_unique<std::uint8_t[]>(sizeof(cells));
            std::memcpy(map.get(), cells, sizeof(cells));
            const foundation::RenderTexturePtr texture = api.rendering->createTexture(foundation::RenderTextureFormat::R8UN, 2, 2, {indexes});
            const util::Description desc = util::Description::parse("textureRowSize : integer = 2");

            api.scene->setCameraLookAt({4.0f, 6.0f, 10.0f}, {1.0f, 0.0f, 1.0f});
            _positioned = api.scene->addVegetation(map, {3, 3, 1}, desc, texture);
            _positioned->setPosition({-2.0f, 0.0f, 0.0f});
            _other = api.scene->addVegetation(map, {3, 3, 2}, desc, texture);
            _other->setPosition({2.0f, 0.0f, 0.0f});
            _hidden = api.scene->addVegetation(map, {3, 3, 1}, desc, texture);
            _hidden->setPosition({2.0f, 0.0f, 0.0f});
            _hidden->setEnabled(false);
        }
        void updateAndDraw(const API &api) override {}
        std::uint8_t getThreshold() const override {
            return 16;
        }

        bool testResult(const API &api, std::size_t outOfThresholdPixelCount) override {
            return outOfThresholdPixelCount <= 128;
        }

    private:
        core::SceneInterface::VegetationPtr _positioned;
        core::SceneInterface::VegetationPtr _other;
        core::SceneInterface::VegetationPtr _hidden;
    };
}

namespace test {
    class scene_ao_disabled_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<scene_ao_disabled_test>();
        }
        void init(const API &api) override {
            api.platform->setSuppressOutput(true, false);
            api.scene->setCameraLookAt({40.0f, 40.0f, 40.0f}, {0.0f, 0.0f, 0.0f});
            api.resourceProvider->getOrLoadVoxelMesh("scene_tests/scene_voxel_mesh_test", [this, scene = api.scene](const std::vector<foundation::RenderDataPtr> &frames, const util::Description &desc) {
                _mesh = scene->addVoxelMesh(frames, desc);
            });
        }
        void updateAndDraw(const API &api) override {}
        std::uint8_t getThreshold() const override {
            return 16;
        }

        bool testResult(const API &api, std::size_t outOfThresholdPixelCount) override {
            return outOfThresholdPixelCount <= 1024;
        }

    private:
        core::SceneInterface::VoxelMeshPtr _mesh;
    };

    class scene_ao_enabled_test : public scene_ao_disabled_test {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<scene_ao_enabled_test>();
        }
        ~scene_ao_enabled_test() override {
            _scene->setAOEnabled(false);
        }
        void init(const API &api) override {
            _scene = api.scene;
            _scene->setAOEnabled(true);
            scene_ao_disabled_test::init(api);
        }

    private:
        core::SceneInterfacePtr _scene;
    };
}

namespace test {
    class scene_voxel_vegetation_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<scene_voxel_vegetation_test>();
        }
        void init(const API &api) override {
            struct Voxel {
                std::int16_t positionX, positionY, positionZ;
                std::uint8_t colorIndex, mask, scaleX, scaleY, scaleZ, reserved;
            };
            const Voxel stone[] = {
                {0, 0, 0, 10, 0x3f, 1, 1, 1, 0},
            };
            const Voxel trunk[] = {
                {0, 0, 0, 40, 0x3f, 1, 1, 1, 0},
                {0, 1, 0, 40, 0x3f, 1, 1, 1, 0},
            };
            const std::uint8_t cells[] = {
                0, 3, 0, 0,   0, 255, 0, 0,   0, 3, 1, 0,
                0, 7, 0, 0,   0, 3, 3, 0,     0, 7, 1, 0,
                0, 3, 1, 0,   0, 255, 1, 0,   0, 3, 2, 0,
            };
            std::vector<std::pair<std::uint32_t, ByteDataPtr>> vxm;
            vxm.emplace_back(1, std::make_unique<std::uint8_t[]>(sizeof(stone)));
            vxm.emplace_back(2, std::make_unique<std::uint8_t[]>(sizeof(trunk)));
            std::memcpy(vxm[0].second.get(), stone, sizeof(stone));
            std::memcpy(vxm[1].second.get(), trunk, sizeof(trunk));
            ByteDataPtr map = std::make_unique<std::uint8_t[]>(sizeof(cells));
            std::memcpy(map.get(), cells, sizeof(cells));
            const util::Description desc = util::Description::parse("geometry : integer = 2");

            api.scene->setCameraLookAt({10.0f, 10.0f, 10.0f}, {0.0f, 0.0f, 0.0f});
            _vegetation = api.scene->addVegetation(map, {3, 3, 3}, desc, vxm);
            _vegetation->setPosition({-1.0f, 0.0f, -1.0f});
            _hidden = api.scene->addVegetation(map, {3, 3, 3}, desc, vxm);
            _hidden->setPosition({-4.0f, 0.0f, 2.0f});
            _hidden->setEnabled(false);
        }
        void updateAndDraw(const API &api) override {}
        std::uint8_t getThreshold() const override {
            return 16;
        }

        bool testResult(const API &api, std::size_t outOfThresholdPixelCount) override {
            return outOfThresholdPixelCount <= 128;
        }

    private:
        core::SceneInterface::VegetationPtr _vegetation;
        core::SceneInterface::VegetationPtr _hidden;
    };
}

namespace test {
    void registerSceneTests(TestRunner &runner) {
        REGISTER_UNIT_TEST(scene_statistics_test);
        REGISTER_UNIT_TEST(scene_camera_look_at_test);
        REGISTER_UNIT_TEST(scene_screen_world_projection_test);
        REGISTER_UNIT_TEST(scene_voxel_mesh_frames_test);
        REGISTER_SCREENSHOT_TEST("scene_tests", scene_line_square_test);
        REGISTER_SCREENSHOT_TEST("scene_tests", scene_custom_mesh_test);
        REGISTER_SCREENSHOT_TEST("scene_tests", scene_voxel_mesh_test);
        REGISTER_SCREENSHOT_TEST("scene_tests", scene_voxel_mesh_scale_test);
        REGISTER_SCREENSHOT_TEST("scene_tests", scene_voxel_mesh_set_frame_test);
#ifndef PLATFORM_WASM
        REGISTER_SCREENSHOT_TEST("scene_tests", scene_voxel_mesh_wireframe_test);
#endif
        REGISTER_SCREENSHOT_TEST("scene_tests", scene_bounding_shapes_test);
        REGISTER_SCREENSHOT_TEST("scene_tests", scene_environment_fog_test);
        REGISTER_SCREENSHOT_TEST("scene_tests", scene_arrows_and_line_set_test);
        REGISTER_SCREENSHOT_TEST("scene_tests", scene_ground_mesh_test);
        REGISTER_SCREENSHOT_TEST("scene_tests", scene_ao_enabled_test);
        REGISTER_SCREENSHOT_TEST("scene_tests", scene_ao_disabled_test);
        REGISTER_SCREENSHOT_TEST("scene_tests", scene_voxel_vegetation_test);
        REGISTER_SCREENSHOT_TEST("scene_tests", scene_textured_vegetation_test);
    }
}
