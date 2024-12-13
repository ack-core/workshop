
#pragma once
#include <game/context.h>
#include <unordered_map>
#include "node_access_interface.h"

namespace game {
    struct EditorNodeStaticMesh : public EditorNode {
        std::string path = "<None>";
        voxel::SceneInterface::StaticMeshPtr mesh;
        
        EditorNodeStaticMesh(std::size_t typeIndex) : EditorNode(typeIndex) {
        
        }
    };

    class EditorStaticMeshContext : public std::enable_shared_from_this<EditorStaticMeshContext>, public Context {
    public:
        EditorStaticMeshContext(API &&api, NodeAccessInterface &nodeAccess);
        ~EditorStaticMeshContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        const NodeAccessInterface &_nodeAccess;
        foundation::EventHandlerToken _editorEventsToken;
        std::unordered_map<std::string, bool (EditorStaticMeshContext::*)(const std::string &)> _handlers;
        
    private:
        bool _selectNode(const std::string &data);
        bool _setPath(const std::string &data);
    };
}
