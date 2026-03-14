
#pragma once
#include <game/context.h>
#include "editor/editor_particles_context.h"

namespace game {
    class DebugContext : public Context {
    public:
        DebugContext(API &&api);
        ~DebugContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        
        foundation::EventHandlerToken _token = foundation::INVALID_EVENT_TOKEN;
        std::size_t _pointerId = foundation::INVALID_POINTER_ID;
        math::vector2f _lockedCoordinates;
        math::vector3f _orbit = { 25, 25, 25 };
        
        core::SceneInterface::LineSetPtr _axis;
        core::WorldInterface::ObjectPtr _knight;
        core::WorldInterface::ObjectPtr _pilon;
        
//        core::SceneInterface::BoundingSpherePtr _bsphere;
//        core::SceneInterface::BoundingBoxPtr _bbox;
        core::SceneInterface::LineSetPtr _rayOut;
    };
}
