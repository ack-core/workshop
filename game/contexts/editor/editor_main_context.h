
#pragma once
#include <game/context.h>
#include <unordered_map>
#include "node_access_interface.h"

namespace game {
    class MovingTool {
    public:
        MovingTool(const API &api, math::vector3f &target);
        ~MovingTool();
        
        void setPosition(const math::vector3f &position);
        const math::vector3f &getPosition() const;
        
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
        
        const std::weak_ptr<EditorNode> &getSelectedNode() const override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        foundation::EventHandlerToken _editorEventsToken;
        voxel::SceneInterface::LineSetPtr _axis;
        
        std::unique_ptr<MovingTool> _movingTool;
        std::unordered_map<std::string, std::shared_ptr<EditorNode>> _nodes;
        std::unordered_map<std::string, bool (EditorMainContext::*)(const std::string &)> _handlers;
        
        std::weak_ptr<EditorNode> _currentNode;
        
//        ui::ImagePtr _testButton1;
//        ui::ImagePtr _testButton2;
        
    private:
        bool _createNode(const std::string &data);
        bool _selectNode(const std::string &data);
        bool _renameNode(const std::string &data);
        bool _moveNodeX(const std::string &data);
        bool _moveNodeY(const std::string &data);
        bool _moveNodeZ(const std::string &data);
        bool _clearNodeSelection(const std::string &data);
    };
}
