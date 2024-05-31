
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
#include "voxel/simulation.h"
#include "voxel/raycast.h"
#include "ui/stage.h"

#include "game/game.h"
#include "game/state_manager.h"
#include "datahub/datahub.h"

foundation::PlatformInterfacePtr platform;
foundation::RenderingInterfacePtr rendering;
resource::ResourceProviderPtr resourceProvider;
voxel::SceneInterfacePtr scene;
voxel::SimulationInterfacePtr simulation;
voxel::RaycastInterfacePtr raycast;
ui::StageInterfacePtr stage;
game::StateManagerPtr stateManager;
dh::DataHubPtr datahub;

extern "C" void initialize() {
    platform = foundation::PlatformInterface::instance();
    rendering = foundation::RenderingInterface::instance(platform);
    resourceProvider = resource::ResourceProvider::instance(platform, rendering);
    scene = voxel::SceneInterface::instance(platform, rendering);
    simulation = voxel::SimulationInterface::instance();
    raycast = voxel::RaycastInterface::instance();
    stage = ui::StageInterface::instance(platform, rendering, resourceProvider);
    datahub = dh::DataHub::instance(platform, game::datahub);
    stateManager = game::StateManager::instance(platform, resourceProvider, scene, simulation, raycast, stage, datahub);
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
}

extern "C" void deinitialize() {

}
