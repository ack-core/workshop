
#include "editor_camera_context.h"

namespace game {    
    EditorCameraContext::EditorCameraContext(API &&api) : _api(std::move(api)) {
        _pointerToken = _api.platform->addPointerEventHandler([this](const foundation::PlatformPointerEventArgs &args) -> bool {
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
            }
            return true;
        });
        
        _keyboardToken = _api.platform->addKeyboardEventHandler([this](const foundation::PlatformKeyboardEventArgs &args) -> bool {
            if (args.key == foundation::Key::W) {
                if (args.type == foundation::PlatformKeyboardEventArgs::EventType::PRESS) {
                    movBF++;
                }
                if (args.type == foundation::PlatformKeyboardEventArgs::EventType::RELEASE) {
                    movBF--;
                }
            }
            if (args.key == foundation::Key::S) {
                if (args.type == foundation::PlatformKeyboardEventArgs::EventType::PRESS) {
                    movBF--;
                }
                if (args.type == foundation::PlatformKeyboardEventArgs::EventType::RELEASE) {
                    movBF++;
                }
            }
            if (args.key == foundation::Key::A) {
                if (args.type == foundation::PlatformKeyboardEventArgs::EventType::PRESS) {
                    movLR--;
                }
                if (args.type == foundation::PlatformKeyboardEventArgs::EventType::RELEASE) {
                    movLR++;
                }
            }
            if (args.key == foundation::Key::D) {
                if (args.type == foundation::PlatformKeyboardEventArgs::EventType::PRESS) {
                    movLR++;
                }
                if (args.type == foundation::PlatformKeyboardEventArgs::EventType::RELEASE) {
                    movLR--;
                }
            }
            return true;
        });
        
        _axis = _api.scene->addLineSet();
        _axis->setLine(0, {-6, 0, -6}, {6, 0, -6}, {0.2, 0.2, 0.2, 0.5});
        _axis->setLine(1, {6, 0, -6}, {6, 0, 6}, {0.2, 0.2, 0.2, 0.5});
        _axis->setLine(2, {6, 0, 6}, {-6, 0, 6}, {0.2, 0.2, 0.2, 0.5});
        _axis->setLine(3, {-6, 0, 6}, {-6, 0, -6}, {0.2, 0.2, 0.2, 0.5});
    }
    
    EditorCameraContext::~EditorCameraContext() {
        _api.platform->removeEventHandler(_pointerToken);
        _api.platform->removeEventHandler(_keyboardToken);
    }
    
    float EditorCameraContext::getOrbitSize() const {
        return _orbit.length();
    }
    
    math::vector3f EditorCameraContext::getTarget() const {
        return _target;
    }

    void EditorCameraContext::update(float dtSec) {
        const math::vector3f right = math::vector3f(0, 1, 0).cross(_orbit).normalized();
        const math::vector3f forward = math::vector3f(0, 1, 0).cross(right).normalized();

        _target = _target + right * float(movLR) * dtSec * 0.1f;
        _target = _target + forward * float(movBF) * dtSec * 0.1f;

        _api.scene->setCameraLookAt(_target + _orbit, _target);
        _axis->setPosition(_target);
    }
}

