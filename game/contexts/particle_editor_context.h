
#pragma once
#include "context.h"

namespace game {
    class ParticleEditorContext : public Context {
    public:
        ParticleEditorContext(API &&api);
        ~ParticleEditorContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        
        foundation::EventHandlerToken _token = foundation::INVALID_EVENT_TOKEN;
        std::size_t _pointerId = foundation::INVALID_POINTER_ID;
        math::vector2f _lockedCoordinates;
        math::vector3f _orbit = { 45, 45, 45 };
        
        voxel::SceneInterface::LineSetPtr _axis;
    };
}
