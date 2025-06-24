
#pragma once
#include <game/context.h>
#include <unordered_map>
#include "node_access_interface.h"
#include "camera_access_interface.h"
#include "editor_moving_tool.h"

// Plan
// v Shape refactor
// v bbox calculation
// v 16bit coords
// v is looped
// v t0_t1_mask_cap
// v edge interpolation
// v smooth stopping (using vertical cap)
// + editor integration
// + setters
// + sphere & box shapes instead disk
// + curves

// Bugs:
// + rename node as existing one -> previous node isn't deleted

namespace game {
    struct Graph {
        auto getFilling(float t) -> float;
    };
    struct Shape {
        enum class Type {
            DISK = 0,
            MESH
        };
        enum class Distribution {
            RANDOM = 0,   // a random point from start shape to a random point of the end shape
            SHUFFLED,     // a random point from start shape to the according point of the end shape
            LINEAR,       // the i-th point from start shape to the according point of the end shape
        };

        Type type;
        float size = 0.0f;
        
        void generate(std::vector<math::vector3f> &points, const math::vector3f &offset, const math::vector3f &dir, std::size_t randomSeed, std::size_t amount);
    };
    
    class Emitter {
    public:
        Emitter();
        ~Emitter();
        
        void setEndShapeOffset(const math::vector3f &offset);
        void refresh(const foundation::RenderingInterfacePtr &rendering);
        
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
        
        bool _isLooped = true;
        std::size_t _randomSeed = 0;

        Shape::Distribution _shapeDistribution = Shape::Distribution::RANDOM;
        Shape _startShape = { Shape::Type::DISK, 0.0f };
        Shape _endShape = { Shape::Type::DISK, 15.0f };
        std::vector<math::vector3f> _startShapePoints;
        std::vector<math::vector3f> _endShapePoints;
        math::vector3f _endShapeOffset = {0, 0, 0};

        foundation::RenderTexturePtr _texture;
        
        std::size_t _bakingFrameTimeMs = 100;
        std::size_t _particlesToEmit = 100;
        
        Graph _emissionGraph;
        float _emissionTimeMs = 1000.0f;
        float _particleLifeTimeMs = 1000.0f; // real lifetime is less by '_bakingFrameTimeMs'
        float _particleSpeed = 10.0f;
        
        std::vector<std::uint8_t> _mapData;
        foundation::RenderTexturePtr _mapTexture;
        voxel::ParticlesParams _ptcParams;
        
    private:
        auto _getShapePoints(float cycleOffset, std::size_t random) const -> std::pair<math::vector3f, math::vector3f>;
    };
}

namespace game {
    struct EditorNodeParticles : public EditorNode {
        std::string texturePath = "<None>";
        foundation::RenderTexturePtr texture;
        Emitter emitter;
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
        
        foundation::RenderTexturePtr _texture;
        
    private:
        void _endShapeDragFinished();
        bool _selectNode(const std::string &data);
        bool _setTexturePath(const std::string &data);
        bool _clearNodeSelection(const std::string &data);
    };
}

