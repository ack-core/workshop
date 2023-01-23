
// Main TODO:
// dynamic model with simplest shader -> scene
//

#include <chrono>

#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/math.h"

#include "voxel/mesh_factory.h"
#include "voxel/scene.h"

#include "ui/stage.h"
#include "game/state_manager.h"

int main(int argc, const char * argv[]) {
    auto platform = foundation::PlatformInterface::instance();
    auto rendering = foundation::RenderingInterface::instance(platform);
    auto meshFactory = voxel::MeshFactory::instance(platform);
    auto scene = voxel::SceneInterface::instance(rendering, meshFactory, "palette.png");
    auto ui = ui::StageInterface::instance(rendering);
    auto states = game::StateManager::instance(scene, ui);
    
    platform->run([&](float dtSec) {
        states->update(dtSec);
        scene->updateAndDraw(dtSec);
        ui->updateAndDraw(dtSec);
        rendering->presentFrame();
    });
    
    return 0;
}

