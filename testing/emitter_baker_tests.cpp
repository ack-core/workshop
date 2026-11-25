
#include "runner.h"
#include "core/emitter_baker.h"
#include <cstring>

namespace test {
    namespace {
        util::Description makeEmitterDesc() {
            util::Description desc;
            desc.setInteger("emissionTimeMs", 1000);
            desc.setInteger("particleLifeTimeMs", 1000);
            desc.setInteger("bakingFrameType", std::int64_t(core::EmitterBaker::BakingTime::BAKE100));
            desc.setInteger("particlesToEmit", 10);
            desc.setBool("looped", false);
            desc.setInteger("randomSeed", 1);
            desc.setInteger("startShapeType", std::int64_t(core::EmitterBaker::ShapeType::BOX));
            desc.setVector3f("startShapeArgs", math::vector3f(1.0f, 0.0f, 0.0f));
            desc.setInteger("endShapeType", std::int64_t(core::EmitterBaker::ShapeType::BOX));
            desc.setVector3f("endShapeArgs", math::vector3f(1.0f, 0.0f, 0.0f));
            desc.setVector3f("endShapeOffset", math::vector3f(0.0f, 10.0f, 0.0f));
            desc.setInteger("shapeDistributionType", std::int64_t(core::EmitterBaker::ShapeDistribution::SHUFFLED));
            desc.setString("emissionGraphData", "0 1 0");
            desc.setString("widthGraphData", "0 1 0");
            desc.setString("heightGraphData", "0 2 0");
            desc.setString("speedGraphData", "0 5 0");
            desc.setString("alphaGraphData", "0 1 0");
            return desc;
        }
    }

    bool emitter_baker_map_test(const API &api) {
        // Map size: width = frames in cycle * cycles in row, height = VERTICAL_PIXELS_PER_PARTICLE * particles (* cycles in row if looped)
        // Map texture: none before refresh, the same one until next refresh
        //
        util::Description desc = makeEmitterDesc();
        core::EmitterBakerPtr baker = core::makeEmitterBaker();
        baker->setParameters(desc);
        EXPECT_EQ(baker->getMap(api.rendering), nullptr);

        baker->refresh();
        EXPECT_EQ(baker->getMapWidth(), 20u);
        EXPECT_EQ(baker->getMapHeight(), core::VERTICAL_PIXELS_PER_PARTICLE * 10u);

        const foundation::RenderTexturePtr map = baker->getMap(api.rendering);
        EXPECT_NOT_EQ(map, nullptr);
        EXPECT_EQ(map, baker->getMap(api.rendering));
        EXPECT_EQ(map->getWidth(), baker->getMapWidth());
        EXPECT_EQ(map->getHeight(), baker->getMapHeight());

        desc.setBool("looped", true);
        baker->setParameters(desc);
        baker->refresh();
        EXPECT_EQ(baker->getMapWidth(), 10u);
        EXPECT_EQ(baker->getMapHeight(), core::VERTICAL_PIXELS_PER_PARTICLE * 10u);
        EXPECT_NOT_EQ(map, baker->getMap(api.rendering));

        desc.setInteger("particleLifeTimeMs", 2500);
        baker->setParameters(desc);
        baker->refresh();
        EXPECT_EQ(baker->getMapWidth(), 30u);
        EXPECT_EQ(baker->getMapHeight(), core::VERTICAL_PIXELS_PER_PARTICLE * 30u);
        return true;
    }

    bool emitter_baker_baking_test(const API &api) {
        // Particles fly from start shape towards end shape, max size comes from width/height graphs
        // Description keeps parameters and follows end shape offset
        // The same parameters give the same map, other random seed gives other map
        //
        auto sameMaps = [](const core::EmitterBakerPtr &a, const core::EmitterBakerPtr &b) {
            const std::size_t size = std::size_t(a->getMapWidth()) * a->getMapHeight() * 4;
            return a->getMapWidth() == b->getMapWidth() && a->getMapHeight() == b->getMapHeight() && std::memcmp(a->getMapRaw(), b->getMapRaw(), size) == 0;
        };

        util::Description desc = makeEmitterDesc();
        core::EmitterBakerPtr baker0 = core::makeEmitterBaker();
        baker0->setParameters(desc);
        baker0->refresh();
        EXPECT_EQ_F(baker0->getMaxSize().x, 1.0f);
        EXPECT_EQ_F(baker0->getMaxSize().y, 2.0f);
        EXPECT_TRUE(baker0->getMaxXYZ().y > 0.0f);
        EXPECT_TRUE(baker0->getMaxXYZ().y <= 5.0f);
        EXPECT_TRUE(baker0->getMinXYZ().x < 0.0f);
        EXPECT_TRUE(baker0->getMaxXYZ().x > 0.0f);
        EXPECT_EQ(baker0->getDescription().getInteger("particlesToEmit", 0), 10);

        baker0->setEndShapeOffset(math::vector3f(1.0f, 2.0f, 3.0f));
        const math::vector3f offset = baker0->getDescription().getVector3f("endShapeOffset", {});
        EXPECT_EQ_F(offset.x, 1.0f);
        EXPECT_EQ_F(offset.y, 2.0f);
        EXPECT_EQ_F(offset.z, 3.0f);

        desc.setInteger("shapeDistributionType", std::int64_t(core::EmitterBaker::ShapeDistribution::RANDOM));
        core::EmitterBakerPtr baker1 = core::makeEmitterBaker();
        baker0->setParameters(desc);
        baker0->refresh();
        baker1->setParameters(desc);
        baker1->refresh();
        EXPECT_TRUE(sameMaps(baker0, baker1));

        desc.setInteger("randomSeed", 2);
        baker1->setParameters(desc);
        baker1->refresh();
        EXPECT_FALSE(sameMaps(baker0, baker1));
        return true;
    }
}

namespace test {
    class emitter_baker_rising_from_square_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<emitter_baker_rising_from_square_test>();
        }
        void init(const API &api) override {
            // Looped emitter: particles rise from filled 10x10 square to the same square 10 units above and fade out
            //
            util::Description desc;
            desc.setInteger("emissionTimeMs", 1000);
            desc.setInteger("particleLifeTimeMs", 1000);
            desc.setInteger("bakingFrameType", std::int64_t(core::EmitterBaker::BakingTime::BAKE20));
            desc.setInteger("particlesToEmit", 200);
            desc.setBool("looped", true);
            desc.setInteger("randomSeed", 1);
            desc.setInteger("startShapeType", std::int64_t(core::EmitterBaker::ShapeType::BOX));
            desc.setBool("startShapeFill", true);
            desc.setVector3f("startShapeArgs", math::vector3f(10.0f, 0.0f, 10.0f));
            desc.setInteger("endShapeType", std::int64_t(core::EmitterBaker::ShapeType::BOX));
            desc.setBool("endShapeFill", true);
            desc.setVector3f("endShapeArgs", math::vector3f(10.0f, 0.0f, 10.0f));
            desc.setVector3f("endShapeOffset", math::vector3f(0.0f, 10.0f, 0.0f));
            desc.setInteger("shapeDistributionType", std::int64_t(core::EmitterBaker::ShapeDistribution::SHUFFLED));
            desc.setInteger("particleOrientation", 1);
            desc.setString("emissionGraphData", "0 1 0");
            desc.setString("widthGraphData", "0 0.5 0 1 0.2 0");
            desc.setString("heightGraphData", "0 0.5 0 1 0.2 0");
            desc.setString("speedGraphData", "0 10 0");
            desc.setString("alphaGraphData", "0 1 0 1 0 0");

            core::EmitterBakerPtr baker = core::makeEmitterBaker();
            baker->setParameters(desc);
            baker->refresh();
            desc.setVector3f("minXYZ", baker->getMinXYZ());
            desc.setVector3f("maxXYZ", baker->getMaxXYZ());
            desc.setVector2f("maxSize", baker->getMaxSize());

            const std::uint32_t white = 0xffffffff;
            const foundation::RenderTexturePtr texture = api.rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, 1, 1, { &white });

            api.scene->setCameraLookAt({20.0f, 20.0f, 20.0f}, {0.0f, 5.0f, 0.0f});
            _particles = api.scene->addParticles(texture, baker->getMap(api.rendering), desc);
            _particles->setTime(0.5f, 0.0f);
        }
        void updateAndDraw(const API &api) override {}
        std::uint8_t getThreshold() const override {
            return 16;
        }
        bool testResult(const API &api, std::size_t outOfThresholdPixelCount) override {
            return outOfThresholdPixelCount <= 128;
        }

    private:
        core::SceneInterface::ParticlesPtr _particles;
    };

    class emitter_baker_expanding_from_disk_test : public ScreenshotTestInterface {
    public:
        static auto make() -> std::unique_ptr<ScreenshotTestInterface> {
            return std::make_unique<emitter_baker_expanding_from_disk_test>();
        }
        void init(const API &api) override {
            // Particles move linearly from small disk outline to the large disk outline 5 units above forming a spiral cone
            //
            util::Description desc;
            desc.setInteger("emissionTimeMs", 1000);
            desc.setInteger("particleLifeTimeMs", 1000);
            desc.setInteger("bakingFrameType", std::int64_t(core::EmitterBaker::BakingTime::BAKE20));
            desc.setInteger("particlesToEmit", 100);
            desc.setBool("looped", false);
            desc.setInteger("randomSeed", 1);
            desc.setInteger("startShapeType", std::int64_t(core::EmitterBaker::ShapeType::DISK));
            desc.setVector3f("startShapeArgs", math::vector3f(2.0f, 0.0f, 0.0f));
            desc.setInteger("endShapeType", std::int64_t(core::EmitterBaker::ShapeType::DISK));
            desc.setVector3f("endShapeArgs", math::vector3f(20.0f, 0.0f, 0.0f));
            desc.setVector3f("endShapeOffset", math::vector3f(0.0f, 5.0f, 0.0f));
            desc.setInteger("shapeDistributionType", std::int64_t(core::EmitterBaker::ShapeDistribution::LINEAR));
            desc.setInteger("particleOrientation", 1);
            desc.setString("emissionGraphData", "0 1 0");
            desc.setString("widthGraphData", "0 0.4 0");
            desc.setString("heightGraphData", "0 0.4 0");
            desc.setString("speedGraphData", "0 10 0");
            desc.setString("alphaGraphData", "0 1 0");

            core::EmitterBakerPtr baker = core::makeEmitterBaker();
            baker->setParameters(desc);
            baker->refresh();
            desc.setVector3f("minXYZ", baker->getMinXYZ());
            desc.setVector3f("maxXYZ", baker->getMaxXYZ());
            desc.setVector2f("maxSize", baker->getMaxSize());

            const std::uint32_t white = 0xffffffff;
            const foundation::RenderTexturePtr texture = api.rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, 1, 1, { &white });

            api.scene->setCameraLookAt({15.0f, 20.0f, 15.0f}, {0.0f, 2.5f, 0.0f});
            _particles = api.scene->addParticles(texture, baker->getMap(api.rendering), desc);
            _particles->setTime(0.9f, 0.0f);
        }
        void updateAndDraw(const API &api) override {}
        std::uint8_t getThreshold() const override {
            return 16;
        }
        bool testResult(const API &api, std::size_t outOfThresholdPixelCount) override {
            return outOfThresholdPixelCount <= 128;
        }

    private:
        core::SceneInterface::ParticlesPtr _particles;
    };
}

// Screenshot tests are named by visual effect: emitter_baker_<motion>_from_<start shape>_test
//
namespace test {
    void registerEmitterBakerTests(TestRunner &runner) {
        REGISTER_UNIT_TEST(emitter_baker_map_test);
        REGISTER_UNIT_TEST(emitter_baker_baking_test);
        REGISTER_SCREENSHOT_TEST("emitter_baker_tests", emitter_baker_rising_from_square_test);
        REGISTER_SCREENSHOT_TEST("emitter_baker_tests", emitter_baker_expanding_from_disk_test);
    }
}
