
#include "debug_context.h"

namespace game {
    template <> std::unique_ptr<Context> makeContext<DebugContext>(API &&api) {
        return std::make_unique<DebugContext>(std::move(api));
    }
    
    DebugContext::DebugContext(API &&api) : _api(std::move(api)) {
        _axis = _api.scene->addLineSet(true, 3);
        _axis->addLine({0.0f, 0.0f, 0.0f}, {1000.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f});
        _axis->addLine({0.0f, 0.0f, 0.0f}, {0.0f, 1000.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f});
        _axis->addLine({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1000.0f}, {0.0f, 0.0f, 1.0f, 1.0f});
        
        _api.yard->setCameraLookAt(_center + _orbit, _center);
        _touchEventHandlerToken = _api.platform->addPointerEventHandler(
            [this](const foundation::PlatformPointerEventArgs &args) -> bool {
                if (args.type == foundation::PlatformPointerEventArgs::EventType::START) {
                    _pointerId = args.pointerID;
                    _lockedCoordinates = { args.coordinateX, args.coordinateY };
                }
                if (args.type == foundation::PlatformPointerEventArgs::EventType::MOVE) {
                    if (_pointerId != foundation::INVALID_POINTER_ID) {
                        float dx = args.coordinateX - _lockedCoordinates.x;
                        float dy = args.coordinateY - _lockedCoordinates.y;
                        
                        _orbit.xz = _orbit.xz.rotated(dx / 100.0f);
                        
                        math::vector3f right = math::vector3f(0, 1, 0).cross(_orbit).normalized();
                        math::vector3f rotatedOrbit = _orbit.rotated(right, dy / 100.0f);
                        
                        if (fabs(math::vector3f(0, 1, 0).dot(rotatedOrbit.normalized())) < 0.96f) {
                            _orbit = rotatedOrbit;
                        }
                        
                        _api.yard->setCameraLookAt(_center + _orbit, _center);
                        _lockedCoordinates = { args.coordinateX, args.coordinateY };
                    }
                }
                if (args.type == foundation::PlatformPointerEventArgs::EventType::FINISH) {
                    _pointerId = foundation::INVALID_POINTER_ID;
                }
                if (args.type == foundation::PlatformPointerEventArgs::EventType::CANCEL) {
                    _pointerId = foundation::INVALID_POINTER_ID;
                }
                
                return true;
            }
        );
        
        _api.yard->addActorType("knight", voxel::YardInterface::ActorTypeDesc {
            .modelPath = "meshes/dev/knight_00",
            .centerPoint = math::vector3f(3, 0, 3),
            .radius = 4.0f,
            .animFPS = 7.0f,
            .animations = {
                {"idle", voxel::YardInterface::ActorTypeDesc::Animation {
                    .firstFrameIndex = 0,
                    .lastFrameIndex = 0
                }},
                {"walk", voxel::YardInterface::ActorTypeDesc::Animation {
                    .firstFrameIndex = 0,
                    .lastFrameIndex = 5
                }}
            }
        });
        _api.yard->addThing("meshes/dev/castle_01", math::vector3f(-50, 0, 0));
        auto actor = _api.yard->addActor("knight", math::vector3f(15, 0, 10), math::vector3f(-1, 0, -1));

        actor->playAnimation("walk", true);
        _api.yard->addStead("textures/stead/floor_00", {32, 0, 0});
    }
    
    DebugContext::~DebugContext() {
        _api.platform->removeEventHandler(_touchEventHandlerToken);
    }
    
    void DebugContext::update(float dtSec) {
    
    }
}
