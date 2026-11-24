
#pragma once
#include <game/context.h>
#include <core/particles.h>
#include <unordered_map>
#include "node_access_interface.h"
#include "camera_access_interface.h"
#include "editor_moving_tool.h"
#include "utils.h"

// Next:
// + particle pivot parameter
// + color graphs
// +

namespace game {
    struct EditorNodeParticles : public EditorNode {
        std::optional<util::Description> currentDesc;
        std::optional<util::Description> originDesc;
        foundation::RenderTexturePtr texture;
        foundation::RenderTexturePtr map;
        core::EmitterBakerPtr emitter;
        core::SceneInterface::ParticlesPtr particles;
        math::vector3f endShapeOffset = {0, 10, 0};
        float currentTime = 0.0f;
        
        EditorNodeParticles(std::size_t typeIndex) : EditorNode(typeIndex) {}
        ~EditorNodeParticles() override {}
        
        void update(float dtSec) override;
        void setResourcePath(const API &api, const std::string &path) override;
    };
    
    class EditorParticlesContext : public Context {
    public:
        EditorParticlesContext(API &&api, NodeAccessInterface &nodeAccess, CameraAccessInterface &cameraAccess);
        ~EditorParticlesContext() override;
        
        void init() override {}
        void update(float dtSec) override;
        
    private:
        const API _api;
        foundation::RenderingInterfacePtr _rendering;
        const CameraAccessInterface &_cameraAccess;
        NodeAccessInterface &_nodeAccess;
        foundation::EventHandlerToken _editorEventsToken;
        std::unordered_map<std::string, bool (EditorParticlesContext::*)(const std::string &)> _handlers;
        
        std::unique_ptr<MovingTool> _endShapeTool; // not null when editing is in progress
        core::SceneInterface::LineSetPtr _shapeConnectLineset;
        core::SceneInterface::LineSetPtr _shapeStartLineset;
        core::SceneInterface::LineSetPtr _shapeEndLineset;
        
        std::string _savingCfg;
        std::unique_ptr<std::uint8_t[]> _savingMap;

    private:
        void _clearEditingTools();
        void _endShapeDragFinished();
        void _refreshShapeLinesets(const core::EmitterBaker &emitter);
        void _recreateParticles(EditorNodeParticles &node, bool editing);
        bool _selectNode(const std::string &data);
        bool _clearNodeSelection(const std::string &data);
        bool _setResourcePath(const std::string &data);
        bool _startEditing(const std::string &data);
        bool _stopEditing(const std::string &data);
        bool _reload(const std::string &data);
        bool _save(const std::string &data);
        bool _emissionSet(const std::string &data);
        bool _visualSet(const std::string &data);
        bool _graphSet(const std::string &data, const std::string &name);
        bool _emissionGraphSet(const std::string &data);
        bool _widthGraphSet(const std::string &data);
        bool _heightGraphSet(const std::string &data);
        bool _speedGraphSet(const std::string &data);
        bool _alphaGraphSet(const std::string &data);
    };
}

