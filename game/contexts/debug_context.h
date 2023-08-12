
#pragma once
#include "context.h"

namespace game {
    class DebugContext : public Context {
    public:
        DebugContext(API &&api);
        ~DebugContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
    
        std::size_t _pointerId = foundation::INVALID_POINTER_ID;
        math::vector2f _lockedCoordinates;

//        math::vector3f _center = { 36, 0, 42 };
//        math::vector3f _orbit = { 0, 80, 45 };

        math::vector3f _center = { 8, 0, 8 };
        math::vector3f _orbit = { 0, 48, 42 };

        foundation::EventHandlerToken _touchEventHandlerToken;
    };
}
