
// Main TODO:
// + python tools: png -> vox, png -> png + palette, 2x large png -> .yard + pngs
// + dh to scene
// + meshProvider/textureProvider need 'update' to remove resources
//
// plan:
// + YardThing
// + palette
// + tool png -> png + palette
// + ui

#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/math.h"

#include "providers/mesh_provider.h"
#include "providers/texture_provider.h"

#include "voxel/scene.h"
#include "voxel/yard.h"

#include "ui/stage.h"

#include "game/state_manager.h"
#include "game/game.h"

#include "datahub/datahub.h"

int main(int argc, const char * argv[]) {
    auto platform = foundation::PlatformInterface::instance();
    auto rendering = foundation::RenderingInterface::instance(platform);
    
    std::unique_ptr<std::uint8_t[]> textureListData;
    std::unique_ptr<std::uint8_t[]> meshListData;

    std::size_t textureListSize = 0;
    std::size_t meshListSize = 0;
    
    bool textureListLoaded = platform->loadFile("textures.list", textureListData, textureListSize);
    bool meshListLoaded = platform->loadFile("meshes.list", meshListData, meshListSize);
    
    if (textureListLoaded && meshListLoaded) {
        std::string texturesList = std::string(reinterpret_cast<const char *>(textureListData.get()), textureListSize);
        std::string meshesList = std::string(reinterpret_cast<const char *>(meshListData.get()), meshListSize);
        
        auto meshProvider = resource::MeshProvider::instance(platform, meshesList.data());
        auto textureProvider = resource::TextureProvider::instance(platform, rendering, texturesList.data());
        auto scene = voxel::SceneInterface::instance(platform, rendering, textureProvider, "palette");
        auto yard = voxel::YardInterface::instance(platform, meshProvider, textureProvider, scene);
        auto ui = ui::StageInterface::instance(platform, rendering, textureProvider);
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
        
        ui->clear();
        
        states = nullptr;
        yard = nullptr;
        scene = nullptr;
        ui = nullptr;
        datahub = nullptr;
        textureProvider = nullptr;
        meshProvider = nullptr;
    }
    
    return 0;
}

