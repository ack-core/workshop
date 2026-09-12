
#pragma once
#include <game/context.h>

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
        math::vector3f _orbit = { 35, 85, 35 };
        core::SceneInterface::LineSetPtr _axis;

        core::SceneInterface::VoxelMeshPtr _knight;
        core::SceneInterface::GroundMeshPtr _ground;
        core::SceneInterface::VegetationPtr _grass;
        core::SceneInterface::VegetationPtr _trees;
        
        std::shared_ptr<ui::StageInterface::Element> _joystick;
        std::shared_ptr<ui::StageInterface::Image> _img0;
        std::shared_ptr<ui::StageInterface::Img9Slice> _img1;
        std::shared_ptr<ui::StageInterface::Element> _btn0;
        std::shared_ptr<ui::StageInterface::TextLine> _txt0;
        std::shared_ptr<ui::StageInterface::TextLine> _txt1;
        std::shared_ptr<ui::StageInterface::TextLine> _txt2;
        std::shared_ptr<ui::StageInterface::TextBlock> _tb0;

    };
}
