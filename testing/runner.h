#pragma once
#include <memory>
#include "providers/resource_provider.h"
#include "core/scene.h"
#include "core/world.h"
#include "core/raycast.h"
#include "ui/stage.h"

#include <unordered_map>

namespace test {
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

        virtual void start() = 0;
        virtual void testResult() = 0;

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

        void registerUnitTest(const char *name, bool (*testFunc)(API&& api));
        void registerStepTest(const char* name, std::unique_ptr<StepTestInterface>(*makeTestInstance)(API&& api));
        void registerScreenshotTest(const char* name, void (*testFunc)(API&& api));

        void nextFrame();

    private:
        std::unordered_map<std::string, bool (*)(API&& api)> _unitTests;
        std::unordered_map<std::string, std::unique_ptr<StepTestInterface>(*)(API&& api)> _stepTests;
        std::unordered_map<std::string, void (*)(API&& api)> _screenshotTests;
    };
}