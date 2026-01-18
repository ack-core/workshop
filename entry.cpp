
#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "providers/resource_provider.h"
#include "voxel/scene.h"
#include "voxel/world.h"
#include "voxel/raycast.h"
#include "voxel/simulation.h"
#include "ui/stage.h"

#include "game/game.h"
#include "game/state_manager.h"
#include "datahub/datahub.h"

foundation::PlatformInterfacePtr platform;
foundation::RenderingInterfacePtr rendering;
resource::ResourceProviderPtr resourceProvider;
voxel::SceneInterfacePtr scene;
voxel::WorldInterfacePtr world;
voxel::RaycastInterfacePtr raycast;
voxel::SimulationInterfacePtr simulation;
ui::StageInterfacePtr stage;
game::StateManagerPtr stateManager;
dh::DataHubPtr datahub;

extern "C" void initialize() {
    platform = foundation::PlatformInterface::instance();
    platform->loadFile(resource::PREFAB_BIN, [](std::unique_ptr<std::uint8_t []> &&prefabsData, std::size_t prefabsSize) {
        rendering = foundation::RenderingInterface::instance(platform);
        resourceProvider = resource::ResourceProvider::instance(platform, rendering, prefabsData, prefabsSize);
        scene = voxel::SceneInterface::instance(platform, rendering);
        world = voxel::WorldInterface::instance(platform, resourceProvider, scene);
        raycast = voxel::RaycastInterface::instance(platform);
        simulation = voxel::SimulationInterface::instance(platform);
        stage = ui::StageInterface::instance(platform, rendering, resourceProvider);
        datahub = dh::DataHub::instance(platform, game::datahub);
        stateManager = game::StateManager::instance(platform, rendering, resourceProvider, scene, world, raycast, simulation, stage, datahub);
        stateManager->switchToState("default");
        
        platform->setLoop([](float dtSec) {
            datahub->update(dtSec);
            stateManager->update(dtSec);
            world->update(dtSec);
            simulation->update(dtSec);
            scene->updateAndDraw(dtSec);
            stage->updateAndDraw(dtSec);
            rendering->presentFrame();
            resourceProvider->update(dtSec);
        });
    });
}

extern "C" void deinitialize() {

}
