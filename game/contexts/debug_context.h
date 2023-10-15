
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

        math::vector3f _center = { 8, 10, 8 };
        math::vector3f _orbit = { 0, 98, 42 };

        foundation::EventHandlerToken _touchEventHandlerToken;
    };
}
