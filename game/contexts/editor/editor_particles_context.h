
#pragma once
#include <game/context.h>
#include <unordered_map>
#include "node_access_interface.h"
#include "camera_access_interface.h"
#include "editor_moving_tool.h"

// Bugs:
// v new random
// v graph spread
// v particle: axis doesnt work
// + baking time -> combo: 10, 20, 100 -> max emission time (devidable by 100)
// ----
// + cycling size graph 1.0 -> 0.0 doesnt work properly with baking = 100
// + particle pivot parameter
// + color graphs
// +

namespace game {
    enum class BakingTime {
        BAKE10 = 1,
        BAKE20,
        BAKE50,
        BAKE100,
    };
    enum class ShapeDistribution {
        RANDOM = 1,   // a random point from start shape to a random point of the end shape
        SHUFFLED,     // a random point from start shape to the corresponding point of the end shape
        LINEAR,       // the i-th point from start shape to the corresponding point of the end shape
    };
    struct RandomSource {
        RandomSource() {}
        RandomSource(std::uint64_t seed, std::uint64_t seq);
        std::uint32_t getNextRandom();
        
        std::uint64_t _state = 0;
        std::uint64_t _inc = 0;
    };
    struct Graph {
        Graph(float absMin, float absMax, float maxSpread, float defaultValue);
        void setPointsFromString(const std::string &data);
        auto getFilling(float t) const -> float;
        auto getValue(float t, float spreadKoeff) const -> float;
        auto getMaxValue() const -> float { return _maxValue; }
        
    public:
        const float _absMin;
        const float _absMax;
        const float _maxSpread;
        
        struct Point {
            float x, lower, upper;
        };
        std::vector<Point> _points;
        float _maxVolume = 0.0f;
        float _maxValue = 0.0f;
    };
    struct Shape {
        enum class Type {
            DISK = 1, // args: radius
            BOX,      // args: x-size, y-size, z-size
            MESH      // args: unused
        };

        Type type = Type::DISK;
        bool fill = false;
        math::vector3f args = {};
        std::vector<math::vector3f> points;
        
        float getMaxSize() const;
        void generate(const voxel::SceneInterface::LineSetPtr &lineSet, const math::vector3f &dir, std::size_t randomSeed, std::size_t amount);
    };
    
    class Emitter {
    public:
        Emitter();
        ~Emitter();
        
        void setParameters(const util::Description &desc);
        void setEndShapeOffset(const math::vector3f &offset);
        void refresh(const foundation::RenderingInterfacePtr &rendering, const voxel::SceneInterface::LineSetPtr &shapeStart, const voxel::SceneInterface::LineSetPtr &shapeEnd);
        
        auto getMap() const -> const foundation::RenderTexturePtr &;
        auto getMapRaw() const -> const std::uint8_t *;
        auto getParams() const -> const voxel::ParticlesParams &;
        
    public:
        struct ActiveParticle {
            const std::size_t index;
            const float bornTimeMs;
            const float lifeTimeMs;
            const math::vector3f start;
            const math::vector3f end;
            math::vector3f currentPosition;
            const float spreadKoeffSize;
            const float spreadKoeffSpeed;
            const float spreadKoeffAlpha;
        };
        
        bool _isLooped = true;
        
        std::size_t _randomSeed = 100;
        RandomSource _shapeGetRandom;

        ShapeDistribution _shapeDistribution = ShapeDistribution::SHUFFLED;
        Shape _startShape = {Shape::Type::DISK, false, {0.0f, 0.0f, 0.0f}};
        Shape _endShape = {Shape::Type::DISK, false, {5.0f, 0.0f, 0.0f}};
        math::vector3f _endShapeOffset = {0, 10.0f, 0};

        foundation::RenderTexturePtr _texture;
        
        std::size_t _bakingFrameTimeMs = 10;
        std::size_t _particlesToEmit = 100;
        
        Graph _emissionGraph = Graph(0.0f, 1.0f, 0.0f, 1.0f);
        float _emissionTimeMs = 1000.0f;
        float _particleLifeTimeMs = 1000.0f; // real lifetime is less by '_bakingFrameTimeMs'
        
        Graph _widthGraph = Graph(0.0f, 99.0f, 99.0f, 1.0f);
        Graph _heightGraph = Graph(0.0f, 99.0f, 99.0f, 1.0f);
        Graph _speedGraph = Graph(-999.0f, +999.0f, 999.0f, 10.0f);
        Graph _alphaGraph = Graph(0.0f, 1.0f, 1.0f, 1.0f);

        std::vector<std::uint8_t> _mapData;
        foundation::RenderTexturePtr _mapTexture;
        voxel::ParticlesParams _ptcParams;
        
    private:
        auto _getShapePoints(float cycleOffset, const math::vector3f &shapeOffset) -> std::pair<math::vector3f, math::vector3f>;
    };
}

namespace game {
    struct EditorNodeParticles : public EditorNode {
        std::optional<util::Description> currentDesc;
        std::optional<util::Description> originDesc;
        foundation::RenderTexturePtr texture;
        foundation::RenderTexturePtr map;
        Emitter emitter;
        voxel::SceneInterface::ParticlesPtr particles;
        math::vector3f endShapeOffset = {0, 10, 0};
        
        EditorNodeParticles(std::size_t typeIndex) : EditorNode(typeIndex) {}
        ~EditorNodeParticles() override {}
        
        void update(float dtSec) override;
        void setResourcePath(const API &api, const std::string &path) override;
    };
    
    class EditorParticlesContext : public Context {
    public:
        EditorParticlesContext(API &&api, NodeAccessInterface &nodeAccess, CameraAccessInterface &cameraAccess);
        ~EditorParticlesContext() override;
        
        void update(float dtSec) override;
        
    private:
        const API _api;
        const CameraAccessInterface &_cameraAccess;
        NodeAccessInterface &_nodeAccess;
        foundation::EventHandlerToken _editorEventsToken;
        std::unordered_map<std::string, bool (EditorParticlesContext::*)(const std::string &)> _handlers;
        
        std::unique_ptr<MovingTool> _endShapeTool; // not null when editing is in progress
        voxel::SceneInterface::LineSetPtr _shapeConnectLineset;
        voxel::SceneInterface::LineSetPtr _shapeStartLineset;
        voxel::SceneInterface::LineSetPtr _shapeEndLineset;
        float currentTime = 0.0f;
        
        std::string _savingCfg;
        std::unique_ptr<std::uint8_t[]> _savingMap;

    private:
        void _clearEditingTools();
        void _endShapeDragFinished();
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

