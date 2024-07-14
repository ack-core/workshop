
#pragma once
#include <game/context.h>
#include <unordered_map>
#include "node_access_interface.h"

namespace game {
    class MovingTool {
    public:
        MovingTool(const API &api, math::vector3f &target);
        ~MovingTool();
        
    public:
        const API &_api;
        math::vector3f &_target;
        voxel::SceneInterface::LineSetPtr _lineset;
        foundation::EventHandlerToken _token = foundation::INVALID_EVENT_TOKEN;
        std::size_t _capturedPointerId = foundation::INVALID_POINTER_ID;
        std::uint32_t _capturedLineIndex = 0;
        float _capturedMovingKoeff = 0.0f;
        math::vector3f _capturedPosition;
    };
        
    class EditorMainContext : public Context, public NodeAccessInterface {
    public:
        EditorMainContext(API &&api);
        ~EditorMainContext() override;
        
        EditorNode *getNode(const std::string &name) const override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        voxel::SceneInterface::LineSetPtr _axis;
        
        std::unique_ptr<MovingTool> _movingTool;
        std::unordered_map<std::string, EditorNode> _nodes;
        
        ui::ImagePtr _testButton1;
        ui::ImagePtr _testButton2;
    };
}
