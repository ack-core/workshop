
#pragma once
#include <game/context.h>
#include <unordered_map>
#include <optional>
#include "node_access_interface.h"

// BUGS:
//
namespace game {
    struct EditorNodeGround : public EditorNode {
        core::SceneInterface::GroundMeshPtr mesh;
        
        EditorNodeGround(std::size_t typeIndex) : EditorNode(typeIndex) {}
        ~EditorNodeGround() override {}
        
        void update(float dtSec) override;
        void setResourcePath(const API &api, const std::string &path) override;
    };

    class EditorGroundContext : public Context {
    public:
        EditorGroundContext(API &&api, NodeAccessInterface &nodeAccess);
        ~EditorGroundContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        NodeAccessInterface &_nodeAccess;
        foundation::EventHandlerToken _editorEventsToken;
        std::unordered_map<std::string, bool (EditorGroundContext::*)(const std::string &)> _handlers;
                
    private:
        bool _selectNode(const std::string &data);
        bool _setResourcePath(const std::string &data);
        bool _startEditing(const std::string &data);
        bool _stopEditing(const std::string &data);
        bool _reload(const std::string &data);
        bool _save(const std::string &data);
    };
}
