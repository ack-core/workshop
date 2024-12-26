
#pragma once
#include <game/context.h>
#include <unordered_map>
#include "node_access_interface.h"
#include "camera_access_interface.h"
#include "editor_moving_tool.h"

namespace game {        
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
        std::unique_ptr<MovingTool> _makeMovingTool(math::vector3f &target);
        
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
