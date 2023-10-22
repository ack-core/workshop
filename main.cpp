
// Main TODO:
// + python tools: xxx large png -> .yard + pngs, check that all png have colors from palette, texture + heightmap + collision -> rgba png
// + dh to scene
// + \n at the end of logMsg
//

#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/math.h"

#include "providers/mesh_provider.h"
#include "providers/texture_provider.h"
#include "providers/fontatlas_provider.h"

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
    std::unique_ptr<std::uint8_t[]> ttfData;

    std::size_t textureListSize = 0;
    std::size_t meshListSize = 0;
    std::size_t ttfSize = 0;
    
    bool textureListLoaded = platform->loadFile("textures.list", textureListData, textureListSize);
    bool meshListLoaded = platform->loadFile("meshes.list", meshListData, meshListSize);
    bool ttfLoaded = platform->loadFile("arial.ttf", ttfData, ttfSize);
    
    if (textureListLoaded && meshListLoaded && ttfLoaded) {
        std::string texturesList = std::string(reinterpret_cast<const char *>(textureListData.get()), textureListSize);
        std::string meshesList = std::string(reinterpret_cast<const char *>(meshListData.get()), meshListSize);
        
        auto meshProvider = resource::MeshProvider::instance(platform, meshesList.data());
        auto textureProvider = resource::TextureProvider::instance(platform, rendering, texturesList.data());
        auto fontAtlasProvider = resource::FontAtlasProvider::instance(platform, rendering, std::move(ttfData), std::uint32_t(ttfSize));
        
        if (foundation::RenderTexturePtr palette = textureProvider->getOrLoadTexture("palette")) {
            auto datahub = dh::DataHub::instance(platform, game::datahub);
            auto scene = voxel::SceneInterface::instance(platform, rendering, textureProvider, palette, datahub);
            auto yard = voxel::YardInterface::instance(platform, rendering, meshProvider, textureProvider, scene);
            auto ui = ui::StageInterface::instance(platform, rendering, textureProvider, fontAtlasProvider);
            auto states = game::StateManager::instance(platform, scene, yard, ui, datahub);
            
            //datahub->getRootScope("graphics")->setBool("drawBBoxes", false);
            
            platform->run([&](float dtSec) {
                textureProvider->update(dtSec);
                meshProvider->update(dtSec);
                fontAtlasProvider->update(dtSec);
                states->update(dtSec);
                datahub->update(dtSec);
                yard->update(dtSec);
                scene->updateAndDraw(dtSec);
                ui->updateAndDraw(dtSec);
                rendering->presentFrame();
            });
            
            ui->clear();
            
            // TODO: errors if destruction is not happened
            states = nullptr;
            yard = nullptr;
            scene = nullptr;
            ui = nullptr;
            datahub = nullptr;
        
        }
        
        fontAtlasProvider = nullptr;
        textureProvider = nullptr;
        meshProvider = nullptr;
    }
    
    return 0;
}

