
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
        math::vector3f _orbit = { 15, 15, 15 };
        
        voxel::SceneInterface::LineSetPtr _axis;
        voxel::SceneInterface::BoundingBoxPtr _bbox;
        voxel::SceneInterface::StaticMeshPtr _thing;
        voxel::SceneInterface::TexturedMeshPtr _ground;
        voxel::SceneInterface::DynamicMeshPtr _actor;
        voxel::SceneInterface::ParticlesPtr _ptc;
        
        Emitter _emitter;
        float _ptcTimeSec = 0.0f;
        float _ptcFiniStamp = -1.0f;
    };
}
