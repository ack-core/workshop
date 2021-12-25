
#include <chrono>

#include "foundation/platform.h"
#include "foundation/rendering.h"

#include "foundation/gears/primitives.h"
#include "foundation/gears/orbit_camera_controller.h"

#include "voxel/mesh_factory.h"
#include "thirdparty/upng/upng.h"

#include "walkers.h"

namespace dbg {
    math::vector3f direction = math::vector3f{};
}

int main(int argc, const char * argv[]) {
    auto platform = foundation::PlatformInterface::instance();
    auto rendering = foundation::RenderingInterface::instance(platform);
    auto camera = std::make_shared<gears::Camera>(platform);

    auto orbitCameraController = std::make_unique<gears::OrbitCameraController>(platform, camera);
    auto primitives = std::make_shared<gears::Primitives>(rendering);

    voxel::MeshFactoryPtr meshFactory = voxel::MeshFactory::instance(platform, rendering, "palette.png");
    game::WalkersPtr walkers = game::Walkers::instance(meshFactory);

    walkers->test();

    platform->addKeyboardEventHandlers(
        [](const foundation::PlatformKeyboardEventArgs &args) {
            if (args.key == foundation::PlatformKeyboardEventArgs::Key::W) {
                dbg::direction.z -= 1;
            }
            if (args.key == foundation::PlatformKeyboardEventArgs::Key::S) {
                dbg::direction.z += 1;
            }
            if (args.key == foundation::PlatformKeyboardEventArgs::Key::A) {
                dbg::direction.x -= 1;
            }
            if (args.key == foundation::PlatformKeyboardEventArgs::Key::D) {
                dbg::direction.x += 1;
            }
        },
        [](const foundation::PlatformKeyboardEventArgs &args) {
            if (args.key == foundation::PlatformKeyboardEventArgs::Key::W) {
                dbg::direction.z += 1;
            }
            if (args.key == foundation::PlatformKeyboardEventArgs::Key::S) {
                dbg::direction.z -= 1;
            }
            if (args.key == foundation::PlatformKeyboardEventArgs::Key::A) {
                dbg::direction.x += 1;
            }
            if (args.key == foundation::PlatformKeyboardEventArgs::Key::D) {
                dbg::direction.x -= 1;
            }
        }
    );

    platform->run([&](float dtSec) {
        rendering->updateCameraTransform(camera->getPosition().flat3, camera->getForwardDirection().flat3, camera->getVPMatrix().flat16);
        rendering->prepareFrame();
        primitives->drawAxis();

        walkers->updateAndDraw(dtSec);

        rendering->presentFrame(dtSec);
    });

    return 0;
}

