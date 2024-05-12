
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
    meshProvider = resource::MeshProvider::instance(platform, rendering);
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
    
    meshProvider->getOrLoadVoxelStatic("statics/ruins", [](const foundation::RenderDataPtr &mesh) {
        if (mesh) {
            thing = scene->addStaticMesh(mesh);
        }
    });
    meshProvider->getOrLoadVoxelObject("objects/stool", [](const std::vector<foundation::RenderDataPtr> &frames) {
        if (frames.size()) {
            actor = scene->addDynamicMesh(frames);
            actor->setTransform(math::transform3f({0, 1, 0}, M_PI / 4).translated({20, 0, 40}));
        }
    });
    groundProvider->getOrLoadGround("grounds/white", [](const foundation::RenderDataPtr &data, const foundation::RenderTexturePtr &texture) {
        if (data && texture) {
            ground = scene->addTexturedMesh(data, texture);
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
        
        textureProvider->update(dtSec);
        meshProvider->update(dtSec);
        groundProvider->update(dtSec);
    });
}

extern "C" void deinitialize() {

}
