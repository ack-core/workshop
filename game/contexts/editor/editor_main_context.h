
#pragma once
#include <game/context.h>
#include <unordered_map>
#include "node_access_interface.h"
#include "camera_access_interface.h"

namespace game {
    class MovingTool {
    public:
        MovingTool(const API &api, const CameraAccessInterface &cameraAccess, math::vector3f &target);
        ~MovingTool();
        
        void setPosition(const math::vector3f &position);
        const math::vector3f &getPosition() const;
        
    public:
        const API &_api;
        const CameraAccessInterface &_cameraAccess;
        math::vector3f &_target;
        voxel::SceneInterface::LineSetPtr _lineset;
        foundation::EventHandlerToken _token = foundation::INVALID_EVENT_TOKEN;
        std::size_t _capturedPointerId = foundation::INVALID_POINTER_ID;
        std::uint32_t _capturedLineIndex = 0;
        math::vector3f _capturedPosition;
        float _capturedMovingKoeff = 0.0f;
        float _toolSize = 5.0f;
    };
        
    class EditorMainContext : public Context, public NodeAccessInterface {
    public:
        EditorMainContext(API &&api, CameraAccessInterface &cameraAccess);
        ~EditorMainContext() override;
        
        const std::weak_ptr<EditorNode> &getSelectedNode() const override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        const CameraAccessInterface &_cameraAccess;
        foundation::EventHandlerToken _editorEventsToken;
        voxel::SceneInterface::LineSetPtr _axis;
        
        std::unique_ptr<MovingTool> _movingTool;
        std::unordered_map<std::string, std::shared_ptr<EditorNode>> _nodes;
        std::unordered_map<std::string, bool (EditorMainContext::*)(const std::string &)> _handlers;
        
        std::weak_ptr<EditorNode> _currentNode;
        
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
