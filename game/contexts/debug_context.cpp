
#include "debug_context.h"
#include <list>

bool dbg_switch = false;
float dbg_rot = 0;

namespace game {
    DebugContext::DebugContext(API &&api) : _api(std::move(api)) {
        _token = _api.platform->addPointerEventHandler(
            [this](const foundation::PlatformPointerEventArgs &args) -> bool {
                if (args.type == foundation::PlatformPointerEventArgs::EventType::START) {
                    _pointerId = args.pointerID;
                    _lockedCoordinates = { args.coordinateX, args.coordinateY };
                }
                if (args.type == foundation::PlatformPointerEventArgs::EventType::MOVE) {
                    if (_pointerId != foundation::INVALID_POINTER_ID) {
                        float dx = args.coordinateX - _lockedCoordinates.x;
                        float dy = args.coordinateY - _lockedCoordinates.y;

                        _orbit.xz = _orbit.xz.rotated(dx / 200.0f);

                        math::vector3f right = math::vector3f(0, 1, 0).cross(_orbit).normalized();
                        math::vector3f rotatedOrbit = _orbit.rotated(right, dy / 200.0f);

                        if (fabs(math::vector3f(0, 1, 0).dot(rotatedOrbit.normalized())) < 0.96f) {
                            _orbit = rotatedOrbit;
                        }

                        _lockedCoordinates = { args.coordinateX, args.coordinateY };
                    }
                }
                if (args.type == foundation::PlatformPointerEventArgs::EventType::FINISH) {
                    dbg_switch = !dbg_switch;
                    _pointerId = foundation::INVALID_POINTER_ID;
                }
                if (args.type == foundation::PlatformPointerEventArgs::EventType::CANCEL) {
                    _pointerId = foundation::INVALID_POINTER_ID;
                }

                return true;
            }
        );

        //_emitter.refresh(_api.rendering, _shapeStart, _shapeEnd);
        
        _axis = _api.scene->addLineSet();
        _axis->setLine(0, {0, 0, 0}, {1000, 0, 0}, {1, 0, 0, 0.9});
        _axis->setLine(1, {0, 0, 0}, {0, 1000, 0}, {0, 1, 0, 0.9});
        _axis->setLine(2, {0, 0, 0}, {0, 0, 1000}, {0, 0, 1, 0.9});
        _axis->setLine(3, {0, 0, 0}, {-1000, 0, 0}, {0.5, 0.5, 0.5, 0.9});
        _axis->setLine(4, {0, 0, 0}, {0, 0, -1000}, {0.5, 0.5, 0.5, 0.9});


    }
    
    DebugContext::~DebugContext() {
        _api.platform->removeEventHandler(_token);
    }
    
    void DebugContext::update(float dtSec) {
        if (!dbg_switch && _object) {
            _object->unloadResources();
            _object = nullptr;
        }
        if (dbg_switch && !_object) {
            _object = _api.world->createObject("player", "prefabs/ship");
            _object->loadResources([](voxel::WorldInterface::Object &) {
                printf("!!! completed !!!\n");
            });
        }
        const math::transform3f trfm = math::transform3f({0, 1, 0}, dbg_rot);
        if (_object) {
            _object->setTransform(trfm);
        }

        _api.scene->setCameraLookAt(_orbit + math::vector3f{0, 0, 0}, {0, 0, 0});
        dbg_rot += dtSec;
        printf("--->>> %f\n", dbg_rot);
    }
}
