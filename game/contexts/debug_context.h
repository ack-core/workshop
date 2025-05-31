
#pragma once
#include <game/context.h>

// Plan
// v Shape refactor
// v bbox calculation
// v 16bit coords
// v is looped
// v t0_t1_mask_cap
// v edge interpolation
// + smooth stopping (using vertical cap)
// + setters
// + editor integration
// + curves

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
        std::size_t _particlesToEmit = 10;
        
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
    class DebugContext : public Context {
    public:
        DebugContext(API &&api);
        ~DebugContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        
        foundation::EventHandlerToken _token = foundation::INVALID_EVENT_TOKEN;
        std::size_t _pointerId = foundation::INVALID_POINTER_ID;
        math::vector2f _lockedCoordinates;
        math::vector3f _orbit = { 15, 15, 15 };
        
        voxel::SceneInterface::LineSetPtr _axis;
        voxel::SceneInterface::BoundingBoxPtr _bbox;
        voxel::SceneInterface::StaticMeshPtr _thing;
        voxel::SceneInterface::TexturedMeshPtr _ground;
        voxel::SceneInterface::DynamicMeshPtr _actor;
        voxel::SceneInterface::ParticlesPtr _ptc;
        
        Emitter _emitter;
        float _ptcTimeSec = 0.0f;
        float _ptcFiniStamp = -1.0f;
    };
}
