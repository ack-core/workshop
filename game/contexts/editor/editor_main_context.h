
#pragma once
#include <game/context.h>
#include <unordered_map>
#include "node_access_interface.h"
#include "camera_access_interface.h"
#include "editor_moving_tool.h"

// Ideas for the buttons at the top:
//

// Editor TODO:
// + reload prefabs on resources change
// + ask before deleting resource
// + button in nodes panel are scrolling with panel -> separate them
// BUGS:
// +

namespace game {
    class EditorMainContext : public Context, public NodeAccessInterface {
    public:
        EditorMainContext(API &&api, CameraAccessInterface &cameraAccess);
        ~EditorMainContext() override;
        
        const std::weak_ptr<EditorNode> &getSelectedNode() const override;
        bool hasNodeWithName(const std::string &name) const override;
        void forEachNode(util::callback<void(const std::shared_ptr<EditorNode> &)> &&handler) override;
        void createNode(core::WorldInterface::NodeType type, const std::string &name, const math::vector3f &position, const std::string &resourcePath) override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        CameraAccessInterface &_cameraAccess;
        foundation::EventHandlerToken _editorEventsToken;
        core::SceneInterface::LineSetPtr _axis;
        
        std::unique_ptr<MovingTool> _movingTool;
        std::unordered_map<std::string, std::shared_ptr<EditorNode>> _nodes;
        std::unordered_map<std::string, bool (EditorMainContext::*)(const std::string &)> _handlers;
        
        std::weak_ptr<EditorNode> _currentNode;
        bool _doUpdateEveryFrame = false;
        bool _isEditing = false;
        
        core::SceneInterface::OctahedronPtr _point;
        core::SceneInterface::BoundingSpherePtr _bsphere;

        
    private:
        std::unique_ptr<MovingTool> _makeMovingTool(math::vector3f &target);
        
    private:
        bool _createNode(const std::string &data);
        bool _deleteNode(const std::string &data);
        bool _selectNode(const std::string &data);
        bool _renameNode(const std::string &data);
        bool _moveNode(const std::string &data);
        bool _clearNodeSelection(const std::string &data);
        bool _startEditing(const std::string &data);
        bool _stopEditing(const std::string &data);
        bool _centerOnObject(const std::string &data);
        bool _axisSwitch(const std::string &data);
        bool _updateSwitch(const std::string &data);
    };
}
