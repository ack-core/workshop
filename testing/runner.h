#pragma once
#include <memory>
#include "providers/resource_provider.h"
#include "core/scene.h"
#include "core/world.h"
#include "core/raycast.h"
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
#define REGISTER_SCREENSHOT_TEST(c) runner.registerScreenshotTest(#c, c::make)

namespace test {
    static const std::size_t MAX_UNIT_TESTS_PER_FRAME = 1;

    struct API {
        const foundation::PlatformInterfacePtr &platform;
        const foundation::RenderingInterfacePtr& rendering;
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
    // @ref   - RGBA pixels of 'rendering_tests/<test name>_ref.tga'
    // @frame - RGBA pixels of the rendered frame
    // Both are TESTING_RENDER_WIDTH x TESTING_RENDER_HEIGHT, rows top-down
    //
    class ScreenshotTestInterface {
    public:
        virtual void init(const API& api) = 0;
        virtual void updateAndDraw(const API& api) = 0;
        virtual bool testResult(const API& api, const std::uint8_t *ref, const std::uint8_t *frame) = 0;

    public:
        virtual ~ScreenshotTestInterface() = default;
    };

    class TestRunner {
    public:
        TestRunner(
            const foundation::PlatformInterfacePtr& platform,
            const foundation::RenderingInterfacePtr& rendering
        );
        ~TestRunner();

    public:

        void registerUnitTest(const char *name, bool (*testFunc)(const API &api));
        void registerStepTest(const char* name, std::unique_ptr<StepTestInterface>(*makeTestInstance)());
        void registerScreenshotTest(const char* name, std::unique_ptr<ScreenshotTestInterface>(*makeTestInstance)());

        void nextFrame();

    private:
        API _api;

        std::vector<std::pair<std::string, bool (*)(const API &api)>> _unitTests;
        std::vector<std::pair<std::string, std::unique_ptr<StepTestInterface>(*)()>> _stepTests;
        std::vector<std::pair<std::string, std::unique_ptr<ScreenshotTestInterface>(*)()>> _screenshotTests;

        int _testsFailed = 0;
        std::size_t _unitTestIndex = 0;
        std::size_t _stepTestIndex = 0;
        std::unique_ptr<StepTestInterface> _currentStepTest;
        std::size_t _screenshotTestIndex = 0;
        std::unique_ptr<ScreenshotTestInterface> _currentScreenshotTest;
        ByteDataPtr _screenshotRef;
        ByteDataPtr _screenshotFrameTGA;
    };
}