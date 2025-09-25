
#pragma once
#include <game/context.h>
#include <unordered_map>
#include "node_access_interface.h"

namespace game {
    struct EditorNodeVoxelMesh : public EditorNode {
        std::string meshPath = "<None>";
        voxel::SceneInterface::VoxelMeshPtr mesh;
        
        EditorNodeVoxelMesh(std::size_t typeIndex) : EditorNode(typeIndex) {}
        ~EditorNodeVoxelMesh() override {}
    };

    class EditorVoxelMeshContext : public std::enable_shared_from_this<EditorVoxelMeshContext>, public Context {
    public:
        EditorVoxelMeshContext(API &&api, NodeAccessInterface &nodeAccess);
        ~EditorVoxelMeshContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        const NodeAccessInterface &_nodeAccess;
        foundation::EventHandlerToken _editorEventsToken;
        std::unordered_map<std::string, bool (EditorVoxelMeshContext::*)(const std::string &)> _handlers;
        
    private:
        bool _selectNode(const std::string &data);
        bool _setMeshPath(const std::string &data);
    };
}
