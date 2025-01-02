
#pragma once
#include <game/context.h>
#include <unordered_map>
#include "node_access_interface.h"
#include "editor_moving_tool.h"

namespace game {
    enum class GraphType {
        CONSTANT
    };
    enum class ShapeType {
        POINT
    };
    
    class Emitter {
    public:
        Emitter();
        ~Emitter();
        
        void setEndShapeOffset(const math::vector3f &offset);
        void refresh();
        
        auto getMap() const -> foundation::RenderTexturePtr;
        auto getParams() const -> const voxel::ParticlesParams &;
        
    private:
        struct ActiveParticle {
            std::size_t index;
            std::size_t randomSeed;
            float bornTimeMs;
            float lifeTimeMs;
            math::vector3f start;
            math::vector3f end;
        };
        
        math::vector3f _endShapeOffset = {0, 1, 0};
        math::vector3f _bbMin = {0, 0, 0};
        math::vector3f _bbMax = {0, 1, 0};
        
        std::size_t _randomSeed = 0;
        voxel::ParticlesOrientation _orientation = voxel::ParticlesOrientation::CAMERA;
        foundation::RenderTexturePtr _texture;
        
        std::uint32_t _bakingFrameTimeMs = 100;
        std::uint32_t _particlesToEmit = 10;
        
        ShapeType _startShapeType;
        ShapeType _endShapeType;
        GraphType _emissionGraphType;
        
        float _emissionTimeMs = 1000.0f;
        float _particleLifeTimeMs = 1000.0f;
        float _particleWidth = 1.0f;
        float _particleHeight = 1.0f;
        
        std::vector<std::uint8_t> _mapData;
        foundation::RenderTexturePtr _mapTexture;
        voxel::ParticlesParams _ptcParams;
        
    private:
        auto _getGraphFilling(GraphType type, float t) const -> float;
        auto _getShapePoints(std::size_t ptcIndex) const -> std::pair<math::vector3f, math::vector3f>;
    };
    
    struct EditorNodeParticles : public EditorNode {
        Emitter emitter;
        std::string texturePath = "<None>";
        foundation::RenderTexturePtr _texture;
        voxel::SceneInterface::ParticlesPtr particles;
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
        void _endShapeDragFinished();
        bool _selectNode(const std::string &data);
        bool _setTexturePath(const std::string &data);
        bool _clearNodeSelection(const std::string &data);
    };
}
