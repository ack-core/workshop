
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
    
        bool _mouseLocked = false;
        math::vector2f _lockedMouseCoordinates;

        math::vector3f _center = { 36, 0, 42 };
        math::vector3f _orbit = { 0, 80, 45 };

        foundation::EventHandlerToken _touchEventHandlerToken;
    };
}
