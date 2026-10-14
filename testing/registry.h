#pragma once
#include <memory>
#include "providers/resource_provider.h"
#include "core/scene.h"
#include "core/world.h"
#include "core/raycast.h"
#include "ui/stage.h"

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

    class RegistryInterface {
    public:
        static std::shared_ptr<RegistryInterface> instance(
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

    public:

        virtual void registerUnitTest(const char *name, bool (*testFunc)(API&& api)) = 0;
        virtual void registerStepTest(const char* name, std::unique_ptr<StepTestInterface>(*testInstance)(API&& api)) = 0;
        virtual void registerScreenshotTest(const char* name, void (*testFunc)(API&& api)) = 0;

        virtual void nextFrame() = 0;

    public:
        virtual ~RegistryInterface() = default;
    };
}