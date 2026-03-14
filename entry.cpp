
#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "providers/resource_provider.h"
#include "core/scene.h"
#include "core/world.h"
#include "core/raycast.h"
#include "core/simulation.h"
#include "ui/stage.h"

#include "game/game.h"
#include "game/state_manager.h"
#include "datahub/datahub.h"

foundation::PlatformInterfacePtr platform;
foundation::RenderingInterfacePtr rendering;
resource::ResourceProviderPtr resourceProvider;
core::SceneInterfacePtr scene;
core::WorldInterfacePtr world;
core::RaycastInterfacePtr raycast;
core::SimulationInterfacePtr simulation;
ui::StageInterfacePtr stage;
game::StateManagerPtr stateManager;
dh::DataHubPtr datahub;

extern "C" void initialize() {
    platform = foundation::PlatformInterface::instance();
    platform->loadFile(resource::PREFAB_BIN, [](std::unique_ptr<std::uint8_t []> &&prefabsData, std::size_t prefabsSize) {
        rendering = foundation::RenderingInterface::instance(platform);
        resourceProvider = resource::ResourceProvider::instance(platform, rendering, prefabsData, prefabsSize);
        scene = core::SceneInterface::instance(platform, rendering);
        raycast = core::RaycastInterface::instance(platform, scene);
        simulation = core::SimulationInterface::instance(platform, scene);
        world = core::WorldInterface::instance(platform, resourceProvider, scene, raycast, simulation);
        stage = ui::StageInterface::instance(platform, rendering, resourceProvider);
        datahub = dh::DataHub::instance(platform, game::datahub);
        stateManager = game::StateManager::instance(platform, resourceProvider, scene, world, raycast, simulation, stage, datahub);
        stateManager->switchToState("default");
        
        platform->setLoop([](float dtSec) {
            datahub->update(dtSec);
            stateManager->update(dtSec);
            world->update(dtSec);
            simulation->update(dtSec);
            raycast->update(dtSec);
            scene->updateAndDraw(dtSec);
            stage->updateAndDraw(dtSec);
            rendering->presentFrame();
            resourceProvider->update(dtSec);
        });
    });
}

extern "C" void deinitialize() {

}
