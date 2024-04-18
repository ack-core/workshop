
/*
Two Contexts:
    YardContext - configures yard according to datahub section
    UIContext - configures ui according to datahub section
    
    datahub section can be filled from text
    yard and ui have callback to fire when they are ready
*/

#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "providers/texture_provider.h"
#include "providers/mesh_provider.h"
#include "providers/ground_provider.h"
#include "voxel/scene.h"

foundation::PlatformInterfacePtr platform;
foundation::RenderingInterfacePtr rendering;
resource::TextureProviderPtr textureProvider;
resource::MeshProviderPtr meshProvider;
resource::GroundProviderPtr groundProvider;
voxel::SceneInterfacePtr scene;

voxel::SceneInterface::BoundingBoxPtr bbox1;
voxel::SceneInterface::LineSetPtr axis;
voxel::SceneInterface::StaticMeshPtr thing;
voxel::SceneInterface::TexturedMeshPtr ground;
voxel::SceneInterface::DynamicMeshPtr actor;


std::size_t pointerId = foundation::INVALID_POINTER_ID;
math::vector2f lockedCoordinates;
math::vector3f orbit = { 45, 45, 45 };

extern "C" void initialize() {
    platform = foundation::PlatformInterface::instance();
    rendering = foundation::RenderingInterface::instance(platform);
    textureProvider = resource::TextureProvider::instance(platform, rendering);
    meshProvider = resource::MeshProvider::instance(platform);
    groundProvider = resource::GroundProvider::instance(platform, rendering);
    scene = voxel::SceneInterface::instance(platform, rendering);
    
    platform->addPointerEventHandler(
        [](const foundation::PlatformPointerEventArgs &args) -> bool {
            if (args.type == foundation::PlatformPointerEventArgs::EventType::START) {
                pointerId = args.pointerID;
                lockedCoordinates = { args.coordinateX, args.coordinateY };
            }
            if (args.type == foundation::PlatformPointerEventArgs::EventType::MOVE) {
                if (pointerId != foundation::INVALID_POINTER_ID) {
                    float dx = args.coordinateX - lockedCoordinates.x;
                    float dy = args.coordinateY - lockedCoordinates.y;
                    
                    orbit.xz = orbit.xz.rotated(dx / 200.0f);
                    
                    math::vector3f right = math::vector3f(0, 1, 0).cross(orbit).normalized();
                    math::vector3f rotatedOrbit = orbit.rotated(right, dy / 200.0f);
                    
                    if (fabs(math::vector3f(0, 1, 0).dot(rotatedOrbit.normalized())) < 0.96f) {
                        orbit = rotatedOrbit;
                    }
                    
                    lockedCoordinates = { args.coordinateX, args.coordinateY };
                }
            }
            if (args.type == foundation::PlatformPointerEventArgs::EventType::FINISH) {
                pointerId = foundation::INVALID_POINTER_ID;
            }
            if (args.type == foundation::PlatformPointerEventArgs::EventType::CANCEL) {
                pointerId = foundation::INVALID_POINTER_ID;
            }
            
            return true;
        }
    );
    
    meshProvider->getOrLoadVoxelMesh("statics/ruins", [](const std::unique_ptr<resource::VoxelMesh> &mesh) {
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

        thing = scene->addStaticMesh(&voxels, 1);
    });
    meshProvider->getOrLoadVoxelMesh("objects/stool", [](const std::unique_ptr<resource::VoxelMesh> &mesh) {
        std::vector<voxel::SceneInterface::VTXDVOX> voxels;
        voxels.reserve(mesh->frames[0].voxelCount);

        for (std::uint16_t i = 0; i < mesh->frames[0].voxelCount; i++) {
            const resource::VoxelMesh::Voxel &src = mesh->frames[0].voxels[i];
            voxel::SceneInterface::VTXDVOX &voxel = voxels.emplace_back();
            voxel.positionX = src.positionX;
            voxel.positionY = src.positionY;
            voxel.positionZ = src.positionZ;
            voxel.colorIndex = src.colorIndex;
            voxel.mask = src.mask;
        }

        actor = scene->addDynamicMesh(&voxels, 1);
        actor->setTransform(math::transform3f({0, 1, 0}, M_PI / 4).translated({20, 0, 40}));
    });
    groundProvider->getOrLoadGround("grounds/white", [](const std::unique_ptr<resource::GroundData> &data) {
        if (data) {
            std::vector<voxel::SceneInterface::VTXNRMUV> vertexes;
            vertexes.reserve(data->vertexes.size());
            
            for (std::size_t i = 0; i < data->vertexes.size(); i++) {
                const resource::GroundData::Vertex &src = data->vertexes[i];
                voxel::SceneInterface::VTXNRMUV &vertex = vertexes.emplace_back();
                vertex.x = src.x;
                vertex.y = src.y;
                vertex.z = src.z;
                vertex.nx = src.nx;
                vertex.ny = src.ny;
                vertex.nz = src.nz;
                vertex.u = src.u;
                vertex.v = src.v;
            }
            
            ground = scene->addTexturedMesh(vertexes, data->indexes, data->texture);
        }
    });
    
    math::bound3f bb1 = {-0.5f, -0.5f, -0.5f, 63.0f + 0.5f, 19.0f + 0.5f, 63.0f + 0.5f};
    bbox1 = scene->addBoundingBox(bb1);
    bbox1->setColor({0.5f, 0.5f, 0.5f, 0.5f});
    
    axis = scene->addLineSet(3);
    axis->setLine(0, {0, 0, 0}, {1000, 0, 0}, {1, 0, 0, 0.5});
    axis->setLine(1, {0, 0, 0}, {0, 1000, 0}, {0, 1, 0, 0.5});
    axis->setLine(2, {0, 0, 0}, {0, 0, 1000}, {0, 0, 1, 0.5});
    
    platform->setLoop([](float dtSec) {
        scene->setCameraLookAt(orbit + math::vector3f{32, 0, 32}, {32, 0, 32});
        scene->updateAndDraw(dtSec);
        rendering->presentFrame();
        meshProvider->update(dtSec);
    });
}

extern "C" void deinitialize() {

}
