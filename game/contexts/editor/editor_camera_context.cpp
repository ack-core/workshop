
#include "editor_camera_context.h"

namespace game {    
    EditorCameraContext::EditorCameraContext(API &&api) : _api(std::move(api)) {
        _token = _api.platform->addPointerEventHandler([this](const foundation::PlatformPointerEventArgs &args) -> bool {
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
                _pointerId = foundation::INVALID_POINTER_ID;
            }
            if (args.type == foundation::PlatformPointerEventArgs::EventType::CANCEL) {
                _pointerId = foundation::INVALID_POINTER_ID;
            }
            if (args.type == foundation::PlatformPointerEventArgs::EventType::WHEEL) {
                float percent = std::max(1000.0f + float(args.flags.wheel), 900.0f) / 1000.0f;
                float orbitLength = _orbit.length();

                if (orbitLength < 50.0f) {
                    if (percent > 1.0f) {
                         _orbit = _orbit * percent;
                    }
                }
                else if (orbitLength > 1000.0f) {
                    if (percent < 1.0f) {
                         _orbit = _orbit * percent;
                    }
                }
                else {
                    _orbit = _orbit * percent;
                }
                
                _api.platform->sendEditorMsg("engine.refresh", "");
            }
            return true;
        });
    }
    
    EditorCameraContext::~EditorCameraContext() {
        _api.platform->removeEventHandler(_token);
    }
    
    float EditorCameraContext::getOrbitSize() const {
        return _orbit.length();
    }

    void EditorCameraContext::update(float dtSec) {
        _api.scene->setCameraLookAt(_orbit, {0, 0, 0});
    }
}

