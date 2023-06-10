
// Main TODO:
// + python tools: png -> vox, png -> png + palette, 2x large png -> .yard + pngs
//

#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/math.h"

#include "voxel/mesh_provider.h"
#include "voxel/texture_provider.h"
#include "voxel/scene.h"
#include "voxel/yard.h"

#include "ui/stage.h"

#include "game/state_manager.h"
#include "game/game.h"

#include "datahub/datahub.h"

int main(int argc, const char * argv[]) {
    auto platform = foundation::PlatformInterface::instance();
    auto rendering = foundation::RenderingInterface::instance(platform);
    auto meshProvider = voxel::MeshProvider::instance(platform);
    auto textureProvider = voxel::TextureProvider::instance(platform, rendering);
    auto scene = voxel::SceneInterface::instance(platform, rendering, textureProvider, "palette");
    auto yard = voxel::YardInterface::instance(platform, meshProvider, textureProvider, scene);
    auto ui = ui::StageInterface::instance(platform, rendering);
    auto datahub = dh::DataHub::instance(platform, game::datahub);
    auto states = game::StateManager::instance(platform, scene, yard, ui, datahub);
    
    platform->run([&](float dtSec) {
        states->update(dtSec);
        datahub->update(dtSec);
        yard->update(dtSec);
        scene->updateAndDraw(dtSec);
        ui->updateAndDraw(dtSec);
        rendering->presentFrame();
    });
    
    return 0;
}

