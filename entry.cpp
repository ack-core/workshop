
#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "providers/texture_provider.h"
#include "providers/mesh_provider.h"
#include "voxel/scene.h"

foundation::PlatformInterfacePtr platform;
foundation::RenderingInterfacePtr rendering;
resource::TextureProviderPtr textureProvider;
resource::MeshProviderPtr meshProvider;
voxel::SceneInterfacePtr scene;

voxel::SceneInterface::BoundingBoxPtr bbox1;
voxel::SceneInterface::LineSetPtr axis;
voxel::SceneInterface::StaticModelPtr thing;

extern "C" void initialize() {
    platform = foundation::PlatformInterface::instance();
    rendering = foundation::RenderingInterface::instance(platform);
    textureProvider = resource::TextureProvider::instance(platform, rendering);
    meshProvider = resource::MeshProvider::instance(platform);
    scene = voxel::SceneInterface::instance(platform, rendering);
    
    meshProvider->getOrLoadVoxelMesh("things/room", resource::MeshOptimization::DISABLED, [](const std::unique_ptr<resource::VoxelMesh> &mesh) {
        std::vector<voxel::SceneInterface::VTXSVOX> voxels;
        voxels.reserve(mesh->frames[0].voxelCount);
        
        for (std::uint16_t i = 0; i < mesh->frames[0].voxelCount; i++) {
            const resource::VoxelMesh::Voxel &src = mesh->frames[0].voxels[i];
            voxel::SceneInterface::VTXSVOX &voxel = voxels.emplace_back();
            voxel.positionX = src.positionX;
            voxel.positionY = src.positionY;
            voxel.positionZ = src.positionZ;
            voxel.colorIndex = src.colorIndex;
            voxel.mask = src.mask;
            voxel.scaleX = src.scaleX;
            voxel.scaleY = src.scaleY;
            voxel.scaleZ = src.scaleZ;
            voxel.reserved = 0;
        }
        
        thing = scene->addStaticModel(voxels);
    });
    
    math::bound3f bb1 = {0.0f, 0.0f, 0.0f, 5.0f, 3.0f, 3.0f};
    bbox1 = scene->addBoundingBox(bb1);
    
    axis = scene->addLineSet(true, 3);
    axis->setLine(0, {0, 0, 0}, {1000, 0, 0}, {1, 0, 0, 1});
    axis->setLine(1, {0, 0, 0}, {0, 1000, 0}, {0, 1, 0, 1});
    axis->setLine(2, {0, 0, 0}, {0, 0, 1000}, {0, 0, 1, 1});
    
    platform->setLoop([](float dtSec) {
        scene->setCameraLookAt({45, 45, 45}, {0, 0, 0});
        scene->updateAndDraw(dtSec);
        rendering->presentFrame();
    });
}

extern "C" void deinitialize() {

}
