
#include <chrono>

#include "foundation/platform/interfaces.h"
#include "foundation/rendering/interfaces.h"

#include "foundation/primitives.h"
#include "foundation/orbit_camera_controller.h"

#include "voxel/mesh_factory/interfaces.h"

#include "thirdparty/upng/upng.h"

namespace {
    math::vector2f wallA{ 1.0f, 2.0f };
    math::vector2f wallB{ 3.0f, 2.0f };
    math::vector2f wallC{ 3.0f, 5.0f };
    math::vector3f position;
    
    int mx = 0, mz = 0;
}

void restrictMovement(math::vector3f &movement, const math::vector3f& origin, float radius, const math::vector2f &wA, const math::vector2f &wB) {
    const math::vector3f nextPosition = origin + movement;
    const math::vector2f wallAB = wB - wA;

    float k = (nextPosition.xz - wA).dot(wallAB) / wallAB.lengthSq();
    math::vector2f kpos;

    if (k <= 0.0f) {
        kpos = wA;
    }
    else if (k >= 1.0f) {
        kpos = wB;
    }
    else {
        kpos = wA + k * wallAB;
    }

    const math::vector3f pp = { nextPosition.x - kpos.x, 0.0f, nextPosition.z - kpos.y };
    const float pplen = pp.length();
    
    if (radius > pplen) {
        float ppkoeff = (radius - pplen) / pplen;
        movement = movement + pp * ppkoeff;
    }
}

int main(int argc, const char * argv[]) {
    auto platform = foundation::PlatformInterface::instance();
    auto rendering = foundation::RenderingInterface::instance(platform);

    auto camera = std::make_shared<gears::Camera>(platform);

    //auto orbitCameraController = std::make_unique<gears::OrbitCameraController>(platform, camera);
    auto primitives = std::make_shared<gears::Primitives>(rendering);

    auto meshes = voxel::MeshFactory::instance(platform, rendering, "palette.png");

    platform->addMouseEventHandlers(
        [](const foundation::PlatformMouseEventArgs &args) {

        },
        [camera](const foundation::PlatformMouseEventArgs &args) {
            const math::vector3f worldDir = camera->screenToWorld(math::vector2f(args.coordinateX, args.coordinateY));
            math::scalar k = -camera->getPosition().y / worldDir.y;

            if (k > 0)
            {
                const math::vector2f worldPos = { camera->getPosition().x + k * worldDir.x, camera->getPosition().z + k * worldDir.z };
                const math::vector2f wallAB = wallB - wallA;

                float k = (worldPos - wallA).dot(wallAB) / wallAB.lengthSq();
                math::vector2f pp;

                if (k <= 0.0f) {
                    pp = wallA;
                }
                else if (k >= 1.0f) {
                    pp = wallB;
                }
                else {
                    pp = wallA + k * wallAB;
                }
                
                printf("-->> %2.2f %2.2f\n", pp.x, pp.y);
            }

        }, 
        [](const foundation::PlatformMouseEventArgs &args) {
        
        }
    );

    platform->addKeyboardEventHandlers(
        [](const foundation::PlatformKeyboardEventArgs &args) {
            if (args.key == foundation::PlatformKeyboardEventArgs::Key::W) {
                mz -= 1;
            }
            if (args.key == foundation::PlatformKeyboardEventArgs::Key::S) {
                mz += 1;
            }
            if (args.key == foundation::PlatformKeyboardEventArgs::Key::A) {
                mx -= 1;
            }
            if (args.key == foundation::PlatformKeyboardEventArgs::Key::D) {
                mx += 1;
            }
        },
        [](const foundation::PlatformKeyboardEventArgs &args) {
            if (args.key == foundation::PlatformKeyboardEventArgs::Key::W) {
                mz += 1;
            }
            if (args.key == foundation::PlatformKeyboardEventArgs::Key::S) {
                mz -= 1;
            }
            if (args.key == foundation::PlatformKeyboardEventArgs::Key::A) {
                mx += 1;
            }
            if (args.key == foundation::PlatformKeyboardEventArgs::Key::D) {
                mx -= 1;
            }
        }
    );

    platform->run([&](float dtSec) {
        math::vector3f movement = math::vector3f(float(mx), 0.0f, float(mz)).normalized(4.0f * dtSec);

        restrictMovement(movement, position, 0.5f, wallB, wallC);
        restrictMovement(movement, position, 0.5f, wallA, wallB);

        position.xz = position.xz + movement.xz;
        camera->setLookAtByRight(position + math::vector3f{ 0.0f, 20.0f, 10.0f }, position, { 1, 0, 0 });

        rendering->updateCameraTransform(camera->getPosition().flat3, camera->getForwardDirection().flat3, camera->getVPMatrix().flat16);
        rendering->prepareFrame();

        primitives->drawAxis();
        primitives->drawLine(math::vector3f(wallA.x, 0, wallA.y), math::vector3f(wallB.x, 0, wallB.y), math::color(1.0f, 1.0f, 1.0f, 1.0f));
        primitives->drawLine(math::vector3f(wallB.x, 0, wallB.y), math::vector3f(wallC.x, 0, wallC.y), math::color(1.0f, 1.0f, 1.0f, 1.0f));

        primitives->drawCircleXZ(position, 0.5f, { 1, 0, 0, 1 });

        rendering->presentFrame(dtSec);
    });

    return 0;
}

