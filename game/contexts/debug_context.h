
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
        math::vector3f _orbit = { 45, 45, 45 };
        
        voxel::SceneInterface::LineSetPtr _axis;
        voxel::SceneInterface::OctahedronPtr _point;
        voxel::SceneInterface::BoundingSpherePtr _bsphere;
        voxel::WorldInterface::ObjectPtr _object;
        
    };
}
