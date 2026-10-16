#pragma once
#include <memory>
#include "providers/resource_provider.h"
#include "core/scene.h"
#include "core/world.h"
#include "core/raycast.h"
#include "ui/stage.h"

#include <string>
#include <vector>

#define EXPECT_EQ(a, b) \
    if ((a) != (b)) { \
        const char *file = __FILE__; \
        int line = __LINE__; \
        api.platform->logMsg("[CHECK FAILED] %s:%d", file, line); \
        return false; \
    }

#define EXPECT_NOT_EQ(a, b) \
    if ((a) == (b)) { \
        const char *file = __FILE__; \
        int line = __LINE__; \
        api.platform->logMsg("[CHECK FAILED] %s:%d", file, line); \
        return false; \
    }

#define EXPECT_TRUE(a) \
    if (!(a)) { \
        const char *file = __FILE__; \
        int line = __LINE__; \
        api.platform->logMsg("[CHECK FAILED] %s:%d", file, line); \
        return false; \
    }

#define EXPECT_FALSE(a) \
    if (a) { \
        const char *file = __FILE__; \
        int line = __LINE__; \
        api.platform->logMsg("[CHECK FAILED] %s:%d", file, line); \
        return false; \
    }

#define EXPECT_LT(a, b) \
    if (!((a) < (b))) { \
        const char *file = __FILE__; \
        int line = __LINE__; \
        api.platform->logMsg("[CHECK FAILED] %s:%d", file, line); \
        return false; \
    }

#define REGISTER_UNIT_TEST(f) runner.registerUnitTest(#f, f)
#define REGISTER_STEP_TEST(c) runner.registerStepTest(#c, c::make)

namespace test {
    static const std::size_t MAX_UNIT_TESTS_PER_FRAME = 1;

    struct API {
        const foundation::PlatformInterfacePtr &platform;
        const foundation::RenderingInterfacePtr& rendering;
        const resource::ResourceProviderPtr &resources;
        const resource::FontAtlasProviderPtr &fonts;
        const core::SceneInterfacePtr &scene;
        const core::WorldInterfacePtr &world;
        const core::RaycastInterfacePtr &raycast;
        const core::SimulationInterfacePtr &simulation;
        const ui::StageInterfacePtr &ui;
    };

    class StepTestInterface {
    public:
        enum class StepType {
            ONE_FRAME,
            ASYNC_WORK_COMPLETE
        };

        virtual auto getType() const -> StepType = 0;
        virtual void init(const API &api) = 0;
        virtual bool testResult(const API &api) = 0;

    public:
        virtual ~StepTestInterface() = default;
    };

    class TestRunner {
    public:
        TestRunner(
            const foundation::PlatformInterfacePtr& platform,
            const foundation::RenderingInterfacePtr& rendering,
            const resource::ResourceProviderPtr& resourceProvider,
            const resource::FontAtlasProviderPtr& fontAtlasProvider,
            const core::SceneInterfacePtr& scene,
            const core::WorldInterfacePtr& world,
            const core::RaycastInterfacePtr& raycast,
            const core::SimulationInterfacePtr& simulation,
            const ui::StageInterfacePtr& ui
        );
        ~TestRunner();

    public:

        void registerUnitTest(const char *name, bool (*testFunc)(const API &api));
        void registerStepTest(const char* name, std::unique_ptr<StepTestInterface>(*makeTestInstance)());
        void registerScreenshotTest(const char* name, void (*testFunc)(const API &api));

        void nextFrame();

    private:
        API _api;

        std::vector<std::pair<std::string, bool (*)(const API &api)>> _unitTests;
        std::vector<std::pair<std::string, std::unique_ptr<StepTestInterface>(*)()>> _stepTests;
        std::vector<std::pair<std::string, void (*)(const API &api)>> _screenshotTests;

        int _testsFailed = 0;
        std::size_t _unitTestIndex = 0;
        std::size_t _stepTestIndex = 0;
        std::unique_ptr<StepTestInterface> _currentStepTest;
    };
}