
#pragma once
#include <game/context.h>
#include <unordered_map>
#include "node_access_interface.h"
#include "camera_access_interface.h"

// BUGS:
//
namespace game {
    struct EditorNodeCollisionShape : public EditorNode {
        
        EditorNodeCollisionShape(std::size_t typeIndex) : EditorNode(typeIndex) {}
        ~EditorNodeCollisionShape() override {}
        
        void update(float dtSec) override;
        void setResourcePath(const API &api, const std::string &path) override;
    };

    class EditorCollisionShapeContext : public Context {
    public:
        EditorCollisionShapeContext(API &&api, NodeAccessInterface &nodeAccess, CameraAccessInterface &cameraAccess);
        ~EditorCollisionShapeContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        const CameraAccessInterface &_cameraAccess;
        NodeAccessInterface &_nodeAccess;
        foundation::EventHandlerToken _editorEventsToken;
        std::unordered_map<std::string, bool (EditorCollisionShapeContext::*)(const std::string &)> _handlers;
        
    private:
        bool _selectNode(const std::string &data);
        bool _setResourcePath(const std::string &data);
        bool _startEditing(const std::string &data);
        bool _stopEditing(const std::string &data);
        bool _reload(const std::string &data);
        bool _save(const std::string &data);
    };
}
