
#pragma once
#include <game/context.h>
#include <unordered_map>
#include "node_access_interface.h"
#include "camera_access_interface.h"

// BUGS:
//
namespace game {
    struct EditorNodeRaycastShape : public EditorNode {
        
        EditorNodeRaycastShape(std::size_t typeIndex) : EditorNode(typeIndex) {}
        ~EditorNodeRaycastShape() override {}
        
        void update(float dtSec) override;
        void setResourcePath(const API &api, const std::string &path) override;
    };

    class EditorRaycastShapeContext : public Context {
    public:
        EditorRaycastShapeContext(API &&api, NodeAccessInterface &nodeAccess, CameraAccessInterface &cameraAccess);
        ~EditorRaycastShapeContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        const CameraAccessInterface &_cameraAccess;
        NodeAccessInterface &_nodeAccess;
        foundation::EventHandlerToken _editorEventsToken;
        std::unordered_map<std::string, bool (EditorRaycastShapeContext::*)(const std::string &)> _handlers;
        
    private:
        bool _selectNode(const std::string &data);
        bool _setResourcePath(const std::string &data);
        bool _startEditing(const std::string &data);
        bool _stopEditing(const std::string &data);
        bool _reload(const std::string &data);
        bool _save(const std::string &data);
    };
}
