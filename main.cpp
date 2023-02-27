
// Main TODO:
// dynamic model with simplest shader -> scene
//

#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/math.h"

#include "voxel/mesh_factory.h"
#include "voxel/scene.h"
#include "voxel/yard.h"

#include "ui/stage.h"

#include "game/state_manager.h"
#include "game/game.h"

#include "datahub/datahub.h"

int main(int argc, const char * argv[]) {
    auto platform = foundation::PlatformInterface::instance();
    auto rendering = foundation::RenderingInterface::instance(platform);
    auto meshFactory = voxel::MeshFactory::instance(platform);
    auto scene = voxel::SceneInterface::instance(platform, rendering, meshFactory, "palette.png");
    auto yard = voxel::YardInterface::instance(platform);
    auto ui = ui::StageInterface::instance(platform, rendering);
    auto datahub = dh::DataHub::instance(platform, game::datahub);
    auto states = game::StateManager::instance(platform, scene, yard, ui, datahub);
    
    platform->run([&](float dtSec) {
        states->update(dtSec);
        datahub->update(dtSec);
        scene->updateAndDraw(dtSec);
        ui->updateAndDraw(dtSec);
        rendering->presentFrame();
    });
    
    return 0;
}

