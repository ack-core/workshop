
#pragma once
#include <game/context.h>
#include <ui/extensions.h>

namespace game {
    class RenderDevContext : public Context {
    public:
        RenderDevContext(API &&api);
        ~RenderDevContext() override;
        
        void init() override {}
        void update(float dtSec) override;
        
    private:
        const API _api;
        
        foundation::EventHandlerToken _token = foundation::INVALID_EVENT_TOKEN;
        std::size_t _pointerId = foundation::INVALID_POINTER_ID;
        math::vector2f _lockedCoordinates;
        math::vector3f _orbit = { 35, 125, 35 };
        core::SceneInterface::LineSetPtr _axis;
    };
}
