#pragma once
#include <memory>
#include "providers/resource_provider.h"
#include "core/scene.h"
#include "core/world.h"
#include "core/raycast.h"
#include "core/simulation.h"
#include "ui/stage.h"

#include <cmath>
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

#define EXPECT_EQ_F(a, b) \
    if (std::fabs(float(a) - float(b)) > 1.0e-5f) { \
        const char *file = __FILE__; \
        int line = __LINE__; \
        api.platform->logMsg("[CHECK FAILED] %s:%d", file, line); \
        return false; \
    }

#define EXPECT_NOT_EQ_F(a, b) \
    if (std::fabs(float(a) - float(b)) <= 1.0e-5f) { \
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
#define REGISTER_PIXEL_TEST(folder, c) runner.registerPixelTest(folder, #c, c::make)
#define REGISTER_SCREENSHOT_TEST(folder, c) runner.registerScreenshotTest(folder, #c, c::make)

namespace test {
    static const std::size_t MAX_UNIT_TESTS_PER_FRAME = 1;

    struct API {
        const foundation::PlatformInterfacePtr &platform;
        const foundation::RenderingInterfacePtr& rendering;
        const resource::ResourceProviderPtr& resourceProvider;
        const core::SceneInterfacePtr& scene;
        const core::RaycastInterfacePtr& raycast;
        const core::SimulationInterfacePtr& simulation;
        const ui::StageInterfacePtr& stage;
    };

    // testResult is called when async work is complete, but not earlier than next frame after init
    //
    class StepTestInterface {
    public:
        virtual void init(const API &api) = 0;
        virtual bool testResult(const API &api) = 0;

    public:
        virtual ~StepTestInterface() = default;
    };

    // testResult is called when async work is complete, but not earlier than next frame after init
    // @ref   - RGBA pixels of '<folder>/<test name>_ref.png'
    // @frame - RGBA pixels of the rendered frame
    // Both are TESTING_RENDER_WIDTH x TESTING_RENDER_HEIGHT, rows top-down
    //
    class PixelTestInterface {
    public:
        virtual void init(const API& api) = 0;
        virtual void updateAndDraw(const API& api) = 0;
        virtual bool testResult(const API& api, const std::uint8_t *ref, const std::uint8_t *frame) = 0;

    public:
        virtual ~PixelTestInterface() = default;
    };

    // Rendered frame is compared with '<folder>/<test name>_ref.png' when async work is complete, but not earlier than next frame after init
    // getThreshold - max allowed absolute difference per RGBA channel, 0 means exact match
    //
    class ScreenshotTestInterface {
    public:
        virtual void init(const API& api) = 0;
        virtual void updateAndDraw(const API& api) = 0;
        virtual std::uint8_t getThreshold() const = 0;

    public:
        virtual ~ScreenshotTestInterface() = default;
    };

    class TestRunner {
    public:
        TestRunner(
            const foundation::PlatformInterfacePtr& platform,
            const foundation::RenderingInterfacePtr& rendering,
            const resource::ResourceProviderPtr& resourceProvider,
            const core::SceneInterfacePtr& scene,
            const core::RaycastInterfacePtr& raycast,
            const core::SimulationInterfacePtr& simulation,
            const ui::StageInterfacePtr& stage
        );
        ~TestRunner();

    public:

        void registerUnitTest(const char *name, bool (*testFunc)(const API &api));
        void registerStepTest(const char* name, std::unique_ptr<StepTestInterface>(*makeTestInstance)());
        void registerPixelTest(const char* folder, const char* name, std::unique_ptr<PixelTestInterface>(*makeTestInstance)());
        void registerScreenshotTest(const char* folder, const char* name, std::unique_ptr<ScreenshotTestInterface>(*makeTestInstance)());

        void nextFrame(float dtSec);

    private:
        void loadScreenshotRef(const std::string &name);
        void saveScreenshot(const std::string &name, const std::uint8_t *frame, const char *suffix);

    private:
        API _api;

        std::vector<std::pair<std::string, bool (*)(const API &api)>> _unitTests;
        std::vector<std::pair<std::string, std::unique_ptr<StepTestInterface>(*)()>> _stepTests;
        std::vector<std::pair<std::string, std::unique_ptr<PixelTestInterface>(*)()>> _pixelTests;
        std::vector<std::pair<std::string, std::unique_ptr<ScreenshotTestInterface>(*)()>> _screenshotTests;

        int _testsFailed = 0;
        std::size_t _unitTestIndex = 0;
        std::size_t _stepTestIndex = 0;
        std::unique_ptr<StepTestInterface> _currentStepTest;
        std::size_t _pixelTestIndex = 0;
        std::unique_ptr<PixelTestInterface> _currentPixelTest;
        std::size_t _screenshotTestIndex = 0;
        std::unique_ptr<ScreenshotTestInterface> _currentScreenshotTest;
        ByteDataPtr _screenshotRef;
    };

    void registerMathTests(TestRunner &runner);
    void registerUtilTests(TestRunner &runner);
    void registerShaderTests(TestRunner &runner);
    void registerPlatformTests(TestRunner &runner);
    void registerRenderingTests(TestRunner &runner);
    void registerResourceProviderTests(TestRunner &runner);
    void registerSceneTests(TestRunner &runner);
    void registerRaycastTests(TestRunner &runner);
    void registerSimulationTests(TestRunner &runner);
    void registerStageTests(TestRunner &runner);
}
