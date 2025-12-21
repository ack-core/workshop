
#pragma once
#include <game/context.h>
#include <unordered_map>
#include "node_access_interface.h"
#include "camera_access_interface.h"
#include "editor_moving_tool.h"

// Ideas for the buttons at the top:
// + center camera pivot to selected object
// + enable/disable 30fps update
//

// Editor TODO:
// + changing prefab (coordinates) -> resave doesnt work
// + reload prefabs on resources change
// + ask before deleting resource
// + separate features in html/js for editor (everything about 'mesh' inside a single function)
// + button in nodes panel are scrolling with panel -> separate them
// BUGS:
// + save prefab works for non-top nodes
// + transform in 3+ hierarchy
// + ctrl + a
// + if right panel is not visible it still catches pointer events -> cant rotate camera

namespace game {
    class EditorMainContext : public Context, public NodeAccessInterface {
    public:
        EditorMainContext(API &&api, CameraAccessInterface &cameraAccess);
        ~EditorMainContext() override;
        
        const std::weak_ptr<EditorNode> &getSelectedNode() const override;
        bool hasNodeWithName(const std::string &name) const override;
        void forEachNode(util::callback<void(const std::shared_ptr<EditorNode> &)> &&handler) override;
        void createNode(EditorNodeType type, const std::string &name, const math::vector3f &position, const std::string &resourcePath) override;
        
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
        bool _deleteNode(const std::string &data);
        bool _selectNode(const std::string &data);
        bool _renameNode(const std::string &data);
        bool _moveNode(const std::string &data);
        bool _clearNodeSelection(const std::string &data);
    };
}
