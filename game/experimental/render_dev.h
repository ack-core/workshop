
#pragma once
#include <game/context.h>

namespace game {
    class RenderDevContext : public Context {
    public:
        RenderDevContext(API &&api);
        ~RenderDevContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        
        foundation::EventHandlerToken _token = foundation::INVALID_EVENT_TOKEN;
        std::size_t _pointerId = foundation::INVALID_POINTER_ID;
        math::vector2f _lockedCoordinates;
        math::vector3f _orbit = { 25, 35, 25 };
        core::SceneInterface::LineSetPtr _axis;

        core::SceneInterface::CustomMeshPtr _mesh0;        
        std::shared_ptr<ui::StageInterface::Element> _joystick;
        std::shared_ptr<ui::StageInterface::Image> _img0;
        std::shared_ptr<ui::StageInterface::Img9Slice> _img1;
    };
}
