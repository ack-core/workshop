
#include <chrono>

#include "foundation/platform.h"
#include "foundation/rendering.h"
#include "foundation/gears/math.h"
#include "foundation/gears/camera.h"
#include "foundation/gears/orbit_camera_controller.h"
#include "foundation/gears/primitives.h"

#include "voxel/mesh_factory.h"
#include "voxel/scene.h"

int main(int argc, const char * argv[]) {
    auto platform = foundation::PlatformInterface::instance();
    auto rendering = foundation::RenderingInterface::instance(platform);
    
    // TODO: move camera management to scene
    auto camera = std::make_shared<gears::Camera>(platform->getScreenWidth(), platform->getScreenHeight());
    
    // TODO: remove, move axis to ???
    auto primitives = std::make_shared<gears::Primitives>(rendering);
    
    auto meshFactory = voxel::MeshFactory::instance(platform);
    auto scene = voxel::Scene::instance(meshFactory, platform, rendering, "palette.png");

    scene->addStaticModel("1.vox", {-16, 0, -16});
    scene->addStaticModel("1.vox", {23, 0, -16});
    scene->addStaticModel("knight.vox", {-12, 1, 6});

    //scene->addLightSource({3.0, 4.0, 3.0}, 12.0f, math::color(1.0, 1.0, 1.0, 1.0));
    
    gears::OrbitCameraController cameraController (platform, camera);
    cameraController.setEnabled(true);
        
    platform->run([&](float dtSec) {
        rendering->updateFrameConstants(
            //camera->getVPMatrix().flat16,
            //camera->getInvViewProjMatrix().flat16,
            camera->getViewMatrix().flat16,
            camera->getProjMatrix().flat16,
            camera->getPosition().flat3,
            camera->getForwardDirection().flat3
        );
        
        //primitives->drawAxis(foundation::RenderPassCommonConfigs::CLEAR(0.8f, 0.775f, 0.75f));

        scene->updateAndDraw(dtSec);
        rendering->presentFrame();
    });

    return 0;
}

