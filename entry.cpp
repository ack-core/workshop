
#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "providers/resource_provider.h"
#include "providers/fontatlas_provider.h"
#include "core/scene.h"
#include "core/world.h"
#include "core/raycast.h"
#include "core/simulation.h"
#include "ui/stage.h"

#if defined(IS_GAME)
#include "game/game.h"
#include "game/state_manager.h"
#elif defined(IS_TESTS)
#include "testing/runner.h"
#include "testing/math_tests.h"
#include "testing/shader_tests.h"
#include "testing/platform_tests.h"
#include "testing/rendering_tests.h"
#endif

foundation::PlatformInterfacePtr platform;
foundation::RenderingInterfacePtr rendering;
resource::ResourceProviderPtr resourceProvider;
resource::FontAtlasProviderPtr fontAtlasProvider;
core::SceneInterfacePtr scene;
core::WorldInterfacePtr world;
core::RaycastInterfacePtr raycast;
core::SimulationInterfacePtr simulation;
ui::StageInterfacePtr stage;

#if defined(IS_GAME)
game::StateManagerPtr stateManager;

extern "C" void initialize() {
    platform = foundation::PlatformInterface::instance();
    platform->loadFile(resource::PREFAB_BIN, [](std::unique_ptr<std::uint8_t[]>&& prefabsData, std::size_t prefabsSize) {
        platform->loadFile("primary.ttf", [prefabsData = std::move(prefabsData), prefabsSize](std::unique_ptr<std::uint8_t[]>&& fontData, std::size_t fontSize) {
            rendering = foundation::RenderingInterface::instance(platform);
            resourceProvider = resource::ResourceProvider::instance(platform, rendering, prefabsData, prefabsSize);
            fontAtlasProvider = resource::FontAtlasProvider::instance(platform, rendering, std::move(fontData), fontSize);
            scene = core::SceneInterface::instance(platform, rendering);
            raycast = core::RaycastInterface::instance(platform, scene);
            simulation = core::SimulationInterface::instance(platform, scene);
            world = core::WorldInterface::instance(platform, resourceProvider, scene, raycast, simulation);
            stage = ui::StageInterface::instance(platform, rendering, resourceProvider, fontAtlasProvider);

            stateManager = game::StateManager::instance(platform, resourceProvider, fontAtlasProvider, scene, world, raycast, simulation, stage);
            stateManager->switchToState("default");

            platform->setLoop([](float dtSec) {
                dtSec = std::min(dtSec, 0.035f);
                stateManager->update(dtSec);
                raycast->update(dtSec);
                simulation->update(dtSec);
                world->update(dtSec);

                scene->updateAndDraw(dtSec);
                stage->updateAndDraw(dtSec);
                rendering->presentFrame();

                resourceProvider->update(dtSec);
                fontAtlasProvider->update(dtSec);
            });
        });
    });
}

extern "C" void deinitialize() {
    stateManager = nullptr;
    stage = nullptr;
    world = nullptr;
    simulation = nullptr;
    raycast = nullptr;
    scene = nullptr;
    fontAtlasProvider = nullptr;
    resourceProvider = nullptr;
    rendering = nullptr;
    platform = nullptr;
}

#elif defined(IS_TESTS)
std::unique_ptr<test::TestRunner> testRunner;

extern "C" void initialize() {
    platform = foundation::PlatformInterface::instance();
    rendering = foundation::RenderingInterface::instance(platform);
    scene = core::SceneInterface::instance(platform, rendering);
    raycast = core::RaycastInterface::instance(platform, scene);
    simulation = core::SimulationInterface::instance(platform, scene);
    
    testRunner = std::make_unique<test::TestRunner>(platform, rendering, resourceProvider, fontAtlasProvider, scene, world, raycast, simulation, stage);
    test::registerMathTests(*testRunner);
    test::registerShaderTests(*testRunner);
    test::registerPlatformTests(*testRunner);
    test::registerRenderingTests(*testRunner);

    platform->setLoop([](float dtSec) {
        testRunner->nextFrame();
    });
}

extern "C" void deinitialize() {
    testRunner = nullptr;
    stage = nullptr;
    world = nullptr;
    simulation = nullptr;
    raycast = nullptr;
    scene = nullptr;
    fontAtlasProvider = nullptr;
    resourceProvider = nullptr;
    rendering = nullptr;
    platform = nullptr;
}
#endif



