
#pragma once
#include <game/context.h>
#include <unordered_map>
#include <optional>
#include "node_access_interface.h"

// BUGS:
//
namespace game {
    struct EditorNodeVoxelMesh : public EditorNode {
        voxel::SceneInterface::VoxelMeshPtr mesh;
        std::unordered_map<std::string, math::vector3i> animations;
        
        EditorNodeVoxelMesh(std::size_t typeIndex) : EditorNode(typeIndex) {}
        ~EditorNodeVoxelMesh() override {}
        
        void update(float dtSec) override;
        void setResourcePath(const API &api, const std::string &path) override;
    };

    class EditorVoxelMeshContext : public Context {
    public:
        EditorVoxelMeshContext(API &&api, NodeAccessInterface &nodeAccess);
        ~EditorVoxelMeshContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        NodeAccessInterface &_nodeAccess;
        foundation::EventHandlerToken _editorEventsToken;
        std::unordered_map<std::string, bool (EditorVoxelMeshContext::*)(const std::string &)> _handlers;
        
        std::string _savingCfg;
        std::optional<float> _currentAnimationTime;
        std::string _currentAnimation;
        bool _animationLooped = false;
        
    private:
        bool _selectNode(const std::string &data);
        bool _setResourcePath(const std::string &data);
        bool _startEditing(const std::string &data);
        bool _stopEditing(const std::string &data);
        bool _reload(const std::string &data);
        bool _meshOffset(const std::string &data);
        bool _save(const std::string &data);
        void _sendCurrentAnimParams(const EditorNodeVoxelMesh &node);
        bool _animationAdd(const std::string &data);
        bool _animationSelected(const std::string &data);
        bool _animationRemoved(const std::string &data);
        bool _animationParameters(const std::string &data);
        bool _animationPlay(const std::string &data);
    };
}
