
/*
Two Contexts:
    WorldContext - configures world according to datahub section
    UIContext - configures ui according to datahub section
    
    datahub section can be filled from text
    yard and ui have callback to fire when they are ready
*/

#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "providers/resource_provider.h"
#include "voxel/scene.h"
#include "voxel/world.h"
#include "voxel/raycast.h"
#include "ui/stage.h"

#include "game/game.h"
#include "game/state_manager.h"
#include "datahub/datahub.h"

foundation::PlatformInterfacePtr platform;
foundation::RenderingInterfacePtr rendering;
resource::ResourceProviderPtr resourceProvider;
voxel::SceneInterfacePtr scene;
voxel::WorldInterfacePtr simulation;
voxel::RaycastInterfacePtr raycast;
ui::StageInterfacePtr stage;
game::StateManagerPtr stateManager;
dh::DataHubPtr datahub;

extern "C" void initialize() {
    platform = foundation::PlatformInterface::instance();
    platform->loadFile(resource::PREFAB_BIN, [](std::unique_ptr<std::uint8_t []> &&data, std::size_t size) {
        rendering = foundation::RenderingInterface::instance(platform);
        resourceProvider = resource::ResourceProvider::instance(platform, rendering, data, size);
        scene = voxel::SceneInterface::instance(platform, rendering);
        simulation = voxel::WorldInterface::instance(platform, resourceProvider, scene);
        raycast = voxel::RaycastInterface::instance(platform, scene);
        stage = ui::StageInterface::instance(platform, rendering, resourceProvider);
        datahub = dh::DataHub::instance(platform, game::datahub);
        stateManager = game::StateManager::instance(platform, rendering, resourceProvider, scene, simulation, raycast, stage, datahub);
        stateManager->switchToState("default");
        
        platform->setLoop([](float dtSec) {
            datahub->update(dtSec);
            stateManager->update(dtSec);
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
