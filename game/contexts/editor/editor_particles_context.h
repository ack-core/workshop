
#pragma once
#include <game/context.h>
#include <unordered_map>
#include "node_access_interface.h"
#include "editor_moving_tool.h"

namespace game {
    class Emitter {
    public:
        Emitter() {}
        ~Emitter() {}
        
        //void setEndShapeOffset(const math::vector3f &offset);
        //auto getEndShapeOffset() const -> math::vector3f;
        
    private:
        math::vector3f _endShapePosition = {0, 1, 0};
        math::vector3f _bbMin = {0, 0, 0};
        math::vector3f _bbMax = {0, 1, 0};
    };
    
    struct EditorNodeParticles : public EditorNode {
        Emitter emitter;
        voxel::SceneInterface::ParticlesPtr particles;
        std::string texturePath = "<None>";
        math::vector3f endShapeOffset = {0, 10, 0};
        
        EditorNodeParticles(std::size_t typeIndex) : EditorNode(typeIndex) {}
        ~EditorNodeParticles() override {}
    };
        
    class EditorParticlesContext : public Context {
    public:
        EditorParticlesContext(API &&api, NodeAccessInterface &nodeAccess, CameraAccessInterface &cameraAccess);
        ~EditorParticlesContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        const NodeAccessInterface &_nodeAccess;
        const CameraAccessInterface &_cameraAccess;
        foundation::EventHandlerToken _editorEventsToken;
        std::unordered_map<std::string, bool (EditorParticlesContext::*)(const std::string &)> _handlers;
        std::unique_ptr<MovingTool> _movingTool;
        voxel::SceneInterface::LineSetPtr _lineset;
        
    private:
        bool _selectNode(const std::string &data);
        bool _setTexturePath(const std::string &data);
        bool _clearNodeSelection(const std::string &data);
    };
}
