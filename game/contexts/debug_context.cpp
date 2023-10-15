
#include "debug_context.h"

namespace game {
    template <> std::unique_ptr<Context> makeContext<DebugContext>(API &&api) {
        return std::make_unique<DebugContext>(std::move(api));
    }
    
    DebugContext::DebugContext(API &&api) : _api(std::move(api)) {
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
        
        _api.yard->addThing("meshes/dev/castle_00", math::vector3f(0, 0, 0));
    }
    
    DebugContext::~DebugContext() {
        _api.platform->removeEventHandler(_touchEventHandlerToken);
    }
    
    void DebugContext::update(float dtSec) {
    
    }
}
