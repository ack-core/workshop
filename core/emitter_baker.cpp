
#include "emitter_baker.h"
#include <list>

namespace {
    const std::uint64_t RND_SHAPE_FILL = 100;
    const std::uint64_t RND_SHAPE_GET = 200;
    const std::uint64_t RND_BORN_PARTICLE = 300;
    const std::uint64_t RND_GRAPH_SPREAD = 400;

    auto getEmitterDir(const math::vector3f &endShapeOffset) -> math::vector3f {
        return endShapeOffset.lengthSq() > 0.1f ? endShapeOffset.normalized() : math::vector3f{0, 1, 0};
    }
    auto getDirRotation(const math::vector3f &dir) -> math::transform3f {
        const float a = math::vector3f(0, 1, 0).angleTo(dir);
        const math::vector3f axis = math::vector3f(0, 1, 0).cross(dir.normalized()).normalized();
        return math::transform3f(axis, -a);
    }
}

namespace {
    using ShapeType = core::EmitterBaker::ShapeType;

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
        const float _defaultValue;

        struct Point {
            float x, lower, upper;
        };
        std::vector<Point> _points;
        float _maxVolume = 0.0f;
        float _maxValue = 0.0f;
    };
    struct Shape {
        ShapeType type = ShapeType::DISK;
        bool fill = false;
        math::vector3f args = {};
        std::vector<math::vector3f> points;

        float getMaxSize() const;
        void generate(const math::vector3f &dir, std::size_t randomSeed, std::size_t amount);
    };

    Graph::Graph(float absMin, float absMax, float maxSpread, float defaultValue) : _absMin(absMin), _absMax(absMax), _maxSpread(maxSpread), _defaultValue(defaultValue) {
        setPointsFromString({});
    }
    void Graph::setPointsFromString(const std::string &data) {
        util::strstream input(data.c_str(), data.length());
        _points.clear();
        _maxValue = 0.0f;
        
        while (!input.isEof()) {
            float x, value, spread;
            if (input >> x >> value >> spread) {
                spread = std::min(spread, _maxSpread);
                const float lower = std::max(value - spread, _absMin);
                const float upper = std::min(value + spread, _absMax);
                _maxValue = std::max(_maxValue, upper);
                _points.emplace_back(Point { std::max(0.0f, std::min(x, 1.0f)), lower, upper });
            }
        }
        if (_points.empty()) {
            _points.emplace_back(Point { 0.0f, _defaultValue, _defaultValue });
            _maxValue = _defaultValue;
        }

        if (_absMin >= 0.0f) {
            _maxVolume = 0.0f;
            for (std::size_t i = 0; i < _points.size() - 1; i++) {
                const float lx = _points[i].x;
                const float rx = _points[i + 1].x;
                const float lupper = _points[i].upper;
                const float rupper = _points[i + 1].upper;
                _maxVolume += 0.5f * (rx - lx) * (lupper + rupper);
            }
            _maxVolume += (1.0f - _points.back().x) * _points.back().upper;
        }
    }
    float Graph::getFilling(float t) const {
        if (_maxVolume < std::numeric_limits<float>::epsilon()) {
            return 1.0f;
        }
        float volume = 0.0f;
        for (std::size_t i = 0; i < _points.size() - 1; i++) {
            const float lx = _points[i].x;
            const float rx = _points[i + 1].x;
            const float lupper = _points[i].upper;
            const float rupper = _points[i + 1].upper;
            
            if (t >= lx && t < rx) {
                const float koeff = (t - lx) / (rx - lx);
                const float tvalue = lupper + (rupper - lupper) * koeff;
                volume += 0.5f * (t - lx) * (lupper + tvalue);
                return volume / _maxVolume;
            }
            else {
                volume += 0.5f * (rx - lx) * (lupper + rupper);
            }
        }
        volume += (t - _points.back().x) * _points.back().upper;
        return volume / _maxVolume;
    }
    auto Graph::getValue(float t, float spreadKoeff) const -> float {
        for (std::size_t i = 0; i < _points.size() - 1; i++) {
            const float lx = _points[i].x;
            const float rx = _points[i + 1].x;
            const float llower = _points[i].lower;
            const float rlower = _points[i + 1].lower;
            const float lupper = _points[i].upper;
            const float rupper = _points[i + 1].upper;

            if (t >= lx && t < rx) {
                const float koeff = (t - lx) / (rx - lx);
                const float lower = llower + (rlower - llower) * koeff;
                const float upper = lupper + (rupper - lupper) * koeff;
                return lower + (upper - lower) * spreadKoeff;
            }
        }
        
        return _points.back().lower + (_points.back().upper - _points.back().lower) * spreadKoeff;
    }
}

namespace {
    float Shape::getMaxSize() const {
        if (type == ShapeType::DISK) {
            return 2.0f * args.x;
        }
        else if (type == ShapeType::BOX) {
            return args.max();
        }
        
        return 0.0f;
    }
    void Shape::generate(const math::vector3f &dir, std::size_t randomSeed, std::size_t amount) {
        util::RandomSource rnd = util::RandomSource(randomSeed, RND_SHAPE_FILL);
        if (type == ShapeType::DISK) {
            points.clear();
            const math::transform3f rotation = getDirRotation(dir);

            // points
            for (std::size_t i = 0; i < amount; i++) {
                const float koeff = 2.0f * MATH_PI * float(i) / float(amount);
                float r = 0.5f * args.x;
                
                if (fill) { // spread in [0..radius]
                    r = float(rnd.getNextRandom() % 991 + 10) / 2000.0f * args.x;
                }
                
                const math::vector3f poff (r * std::cosf(koeff), 0.0f, r * std::sinf(koeff));
                points.emplace_back(poff.transformed(rotation)); //
            }
        }
        else if (type == ShapeType::BOX) {
            points.clear();

            int dimIndexes[3];
            int dimCount = 0;
            
            if (args.x > std::numeric_limits<float>::epsilon()) {
                dimIndexes[dimCount++] = 0;
            }
            if (args.y > std::numeric_limits<float>::epsilon()) {
                dimIndexes[dimCount++] = 1;
            }
            if (args.z > std::numeric_limits<float>::epsilon()) {
                dimIndexes[dimCount++] = 2;
            }
            
            if (fill) {
                if (dimCount == 1) {
                    for (std::size_t i = 0; i < amount; i++) {
                        float koeff = float(i) / float(amount - 1) - 0.5f;
                        points.emplace_back(math::vector3f(args.x * koeff, args.y * koeff, args.z * koeff));
                    }
                }
                else {
                    for (std::size_t i = 0; i < amount; i++) {
                        math::vector3f koeff;
                        koeff.x = float(rnd.getNextRandom() % 10001) / float(10000) - 0.5f;
                        koeff.y = float(rnd.getNextRandom() % 10001) / float(10000) - 0.5f;
                        koeff.z = float(rnd.getNextRandom() % 10001) / float(10000) - 0.5f;
                        points.emplace_back(args * koeff);
                    }
                }
            }
            else {
                if (dimCount == 1) {
                    for (std::size_t i = 0; i < amount; i++) {
                        float koeff = float(i) / float(amount - 1) - 0.5f;
                        points.emplace_back(args * koeff);
                    }
                }
                else if (dimCount == 2) {
                    for (int i = 0; i < int(amount) / 4; i++) {
                        math::vector3f koeff = {0};
                        koeff.flat[dimIndexes[0]] = float(i) / float(amount / 4 - 1) - 0.5f;
                        koeff.flat[dimIndexes[1]] = -0.5f;
                        points.emplace_back(args * koeff);
                    }
                    for (int i = 0; i < int(amount) / 4; i++) {
                        math::vector3f koeff = {0};
                        koeff.flat[dimIndexes[0]] = 0.5f;
                        koeff.flat[dimIndexes[1]] = float(i) / float(amount / 4 - 1) - 0.5f;
                        points.emplace_back(args * koeff);
                    }
                    for (int i = int(amount) / 4 - 1; i >= 0; i--) {
                        math::vector3f koeff = {0};
                        koeff.flat[dimIndexes[0]] = float(i) / float(amount / 4 - 1) - 0.5f;
                        koeff.flat[dimIndexes[1]] = 0.5f;
                        points.emplace_back(args * koeff);
                    }
                    for (int i = int(amount) / 4 - 1; i >= 0; i--) {
                        math::vector3f koeff = {0};
                        koeff.flat[dimIndexes[0]] = -0.5f;
                        koeff.flat[dimIndexes[1]] = float(i) / float(amount / 4 - 1) - 0.5f;
                        points.emplace_back(args * koeff);
                    }
                }
                else {
                    for (std::size_t i = 0; i < amount; i++) {
                        math::vector3f koeff;
                        koeff.x = float(rnd.getNextRandom() % 10001) / float(10000) - 0.5f;
                        koeff.y = float(rnd.getNextRandom() % 10001) / float(10000) - 0.5f;
                        koeff.z = float(rnd.getNextRandom() % 10001) / float(10000) - 0.5f;
                        
                        if (dimCount) {
                            koeff.flat[dimIndexes[i % dimCount]] = rnd.getNextRandom() % 2000 >= 1000 ? 0.5f : -0.5f;
                        }
                        
                        points.emplace_back(args * koeff);
                    }
                }

            }
        }
    }
}

namespace core {
    using ShapeDistribution = EmitterBaker::ShapeDistribution;

    class EmitterBakerImpl : public EmitterBaker {
    public:
        EmitterBakerImpl() {
            _description.setInteger("emissionTimeMs", std::int64_t(_emissionTimeMs));
            _description.setInteger("particleLifeTimeMs", std::int64_t(_particleLifeTimeMs));
            _description.setInteger("bakingFrameType", std::int64_t(_bakingFrameType));
            _description.setInteger("particlesToEmit", std::int64_t(_particlesToEmit));
            _description.setBool("looped", _isLooped);
            _description.setInteger("randomSeed", std::int64_t(_randomSeed));
            _description.setInteger("startShapeType", std::int64_t(_startShape.type));
            _description.setBool("startShapeFill", _startShape.fill);
            _description.setVector3f("startShapeArgs", _startShape.args);
            _description.setInteger("endShapeType", std::int64_t(_endShape.type));
            _description.setBool("endShapeFill", _endShape.fill);
            _description.setVector3f("endShapeArgs", _endShape.args);
            _description.setVector3f("endShapeOffset", _endShapeOffset);
            _description.setInteger("shapeDistributionType", std::int64_t(_shapeDistribution));
        }

        void setParameters(const util::Description &desc) override;
        void setEndShapeOffset(const math::vector3f &offset) override;
        void refresh() override;

        auto getMap(const foundation::RenderingInterfacePtr &rendering) -> const foundation::RenderTexturePtr & override;
        auto getMapRaw() const -> const std::uint8_t * override { return _mapData.data(); }
        auto getMapWidth() const -> std::uint32_t override { return _mapWidth; }
        auto getMapHeight() const -> std::uint32_t override { return _mapHeight; }
        auto getDescription() const -> const util::Description & override { return _description; }

        auto getMinXYZ() const -> const math::vector3f & override { return _minXYZ; }
        auto getMaxXYZ() const -> const math::vector3f & override { return _maxXYZ; }
        auto getMaxSize() const -> const math::vector2f & override { return _maxSize; }

    private:
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

        std::size_t _randomSeed = 0;
        util::RandomSource _shapeGetRandom;

        ShapeDistribution _shapeDistribution = ShapeDistribution::RANDOM;
        Shape _startShape = {ShapeType::DISK, false, {0.0f, 0.0f, 0.0f}};
        Shape _endShape = {ShapeType::DISK, false, {0.0f, 0.0f, 0.0f}};
        math::vector3f _endShapeOffset = {0, 10.0f, 0};

        BakingTime _bakingFrameType = BakingTime::BAKE10;
        std::size_t _particlesToEmit = 5;

        Graph _emissionGraph = Graph(0.0f, 1.0f, 0.0f, 1.0f);
        float _emissionTimeMs = 1000.0f;
        float _particleLifeTimeMs = 1000.0f; // real lifetime is less by baking frame time

        Graph _widthGraph = Graph(0.0f, 99.0f, 99.0f, 1.0f);
        Graph _heightGraph = Graph(0.0f, 99.0f, 99.0f, 1.0f);
        Graph _speedGraph = Graph(-999.0f, +999.0f, 999.0f, 10.0f);
        Graph _alphaGraph = Graph(0.0f, 1.0f, 1.0f, 1.0f);

        std::vector<std::uint8_t> _mapData;
        std::uint32_t _mapWidth = 0;
        std::uint32_t _mapHeight = 0;
        foundation::RenderTexturePtr _mapTexture;
        util::Description _description;

        math::vector3f _minXYZ = {0, 0, 0};
        math::vector3f _maxXYZ = {0, 0, 0};
        math::vector2f _maxSize = {0, 0};

    private:
        auto _getShapePoints(float cycleOffset, const math::vector3f &shapeOffset) -> std::pair<math::vector3f, math::vector3f>;
    };

    void EmitterBakerImpl::setParameters(const util::Description &params) {
        _description.setInteger("emissionTimeMs", params.getInteger("emissionTimeMs", std::int64_t(_emissionTimeMs)));
        _description.setInteger("particleLifeTimeMs", params.getInteger("particleLifeTimeMs", std::int64_t(_particleLifeTimeMs)));
        _description.setInteger("bakingFrameType", params.getInteger("bakingFrameType", std::int64_t(_bakingFrameType)));
        _description.setInteger("particlesToEmit", params.getInteger("particlesToEmit", std::int64_t(_particlesToEmit)));
        _description.setBool("looped", params.getBool("looped", _isLooped));
        _description.setInteger("randomSeed", params.getInteger("randomSeed", std::int64_t(_randomSeed)));
        _description.setInteger("startShapeType", params.getInteger("startShapeType", std::int64_t(_startShape.type)));
        _description.setBool("startShapeFill", params.getBool("startShapeFill", _startShape.fill));
        _description.setVector3f("startShapeArgs", params.getVector3f("startShapeArgs", _startShape.args));
        _description.setInteger("endShapeType", params.getInteger("endShapeType", std::int64_t(_endShape.type)));
        _description.setBool("endShapeFill", params.getBool("endShapeFill", _endShape.fill));
        _description.setVector3f("endShapeArgs", params.getVector3f("endShapeArgs", _endShape.args));
        _description.setVector3f("endShapeOffset", params.getVector3f("endShapeOffset", _endShapeOffset));
        _description.setInteger("shapeDistributionType", params.getInteger("shapeDistributionType", std::int64_t(_shapeDistribution)));
        _description.setString("emissionGraphData", params.getString("emissionGraphData", _description.getString("emissionGraphData", "")));
        _description.setString("widthGraphData", params.getString("widthGraphData", _description.getString("widthGraphData", "")));
        _description.setString("heightGraphData", params.getString("heightGraphData", _description.getString("heightGraphData", "")));
        _description.setString("speedGraphData", params.getString("speedGraphData", _description.getString("speedGraphData", "")));
        _description.setString("alphaGraphData", params.getString("alphaGraphData", _description.getString("alphaGraphData", "")));
        _description.setInteger("particleOrientation", params.getInteger("particleOrientation", _description.getInteger("particleOrientation", 0)));
        _description.setBool("additiveBlend", params.getBool("additiveBlend", _description.getBool("additiveBlend", false)));
        _description.setString("texture", params.getString("texture", _description.getString("texture", "")));

        _emissionTimeMs = float(_description.getInteger("emissionTimeMs", 0));
        _particleLifeTimeMs = float(_description.getInteger("particleLifeTimeMs", 0));
        _bakingFrameType = BakingTime(_description.getInteger("bakingFrameType", 0));
        _particlesToEmit = std::size_t(_description.getInteger("particlesToEmit", 0));
        _isLooped = _description.getBool("looped", false);
        _randomSeed = std::size_t(_description.getInteger("randomSeed", 0));
        _startShape.type = ShapeType(_description.getInteger("startShapeType", 0));
        _startShape.fill = _description.getBool("startShapeFill", false);
        _startShape.args = _description.getVector3f("startShapeArgs", {});
        _endShape.type = ShapeType(_description.getInteger("endShapeType", 0));
        _endShape.fill = _description.getBool("endShapeFill", false);
        _endShape.args = _description.getVector3f("endShapeArgs", {});
        _endShapeOffset = _description.getVector3f("endShapeOffset", {});
        _shapeDistribution = ShapeDistribution(_description.getInteger("shapeDistributionType", 0));
        _emissionGraph.setPointsFromString(_description.getString("emissionGraphData", ""));
        _widthGraph.setPointsFromString(_description.getString("widthGraphData", ""));
        _heightGraph.setPointsFromString(_description.getString("heightGraphData", ""));
        _speedGraph.setPointsFromString(_description.getString("speedGraphData", ""));
        _alphaGraph.setPointsFromString(_description.getString("alphaGraphData", ""));
    }
    
    void EmitterBakerImpl::setEndShapeOffset(const math::vector3f &offset) {
        _endShapeOffset = offset;
        _description.setVector3f("endShapeOffset", offset);
    }

    void EmitterBakerImpl::refresh() {
        const std::size_t bakingTimeTable[] = {
            10, 10, 20, 50, 100
        };
        const std::size_t bakingFrameTimeMs = bakingTimeTable[std::size_t(_bakingFrameType)];

        _shapeGetRandom = util::RandomSource(_randomSeed, RND_SHAPE_GET);

        const math::vector3f emitterDir = getEmitterDir(_endShapeOffset);
        const std::size_t cycleLength = std::size_t(std::ceil(_emissionTimeMs / float(bakingFrameTimeMs)));
        const std::size_t cyclesInRow = std::size_t(std::ceil((_isLooped ? std::max(_emissionTimeMs, _particleLifeTimeMs) : _emissionTimeMs + _particleLifeTimeMs) / _emissionTimeMs));
        
        _startShape.generate(emitterDir, _randomSeed, 1024);
        _endShape.generate(emitterDir, _randomSeed, 1024);
        
        const std::uint32_t mapTextureWidth = std::uint32_t(cyclesInRow * cycleLength);
        const std::uint32_t mapVerticalCount = std::uint32_t(_particlesToEmit * (_isLooped ? cyclesInRow : 1));
        const std::uint32_t mapTextureHeight = core::VERTICAL_PIXELS_PER_PARTICLE * mapVerticalCount;
        
        struct MapElement {
            math::vector3f position;
            float width, height;
            float alpha, angle;
            std::uint8_t mask = 128, history;
        };
        std::vector<MapElement> tmpMap;
        tmpMap.resize(mapTextureWidth * mapVerticalCount);

        _mapData.resize(mapTextureWidth * mapTextureHeight * 4);
        std::fill(_mapData.begin(), _mapData.end(), 128);
        
        std::list<ActiveParticle> activeParticles;
        std::size_t bornParticleCount = 0;
        
        _minXYZ = {0, 0, 0};
        _maxXYZ = {0, 0, 0};
        _maxSize = {_widthGraph.getMaxValue(), _heightGraph.getMaxValue()};

        util::RandomSource rndSpread = util::RandomSource(RND_GRAPH_SPREAD, _randomSeed);
        
        // second loop is needed to bake old particles from previous cycle (_isLooped)
        for (std::size_t loop = 0; loop < std::size_t(_isLooped) + 1; loop++) { //
            for (std::size_t i = 0; i < mapTextureWidth; i++) {
                const std::size_t timeIndex = i + loop * mapTextureWidth;
                const float currAbsTimeMs = float((timeIndex + 0) * bakingFrameTimeMs);
                const float nextAbsTimeMs = float((timeIndex + 1) * bakingFrameTimeMs);
                const float currEmissionTimeKoeff = float(((timeIndex % cycleLength) + 0) * bakingFrameTimeMs) / float(_emissionTimeMs);
                const float nextEmissionTimeKoeff = float(((timeIndex % cycleLength) + 1) * bakingFrameTimeMs) / float(_emissionTimeMs);
                                
                for (auto ptc = activeParticles.begin(); ptc != activeParticles.end(); ) {
                    if (nextAbsTimeMs - ptc->bornTimeMs > ptc->lifeTimeMs) {
                        ptc = activeParticles.erase(ptc);
                    }
                    else {
                        ++ptc;
                    }
                }
                
                if (loop == 0) {
                    float emissinGraphFilling = _emissionGraph.getFilling(nextEmissionTimeKoeff);
                    const std::size_t nextEmissionGraphValue = std::size_t(std::ceil(float(_particlesToEmit) * emissinGraphFilling));
                    const std::size_t nextParticleCount = std::min(i / cycleLength * _particlesToEmit + nextEmissionGraphValue, _isLooped ? std::size_t(-1) : _particlesToEmit);
                    
                    for (std::size_t c = bornParticleCount; c < nextParticleCount; c++) {
                        const float ptcEmissionKoeff = float(c) / float(_particlesToEmit);
                        const std::pair<math::vector3f, math::vector3f> shapePoints = _getShapePoints(ptcEmissionKoeff, _endShapeOffset);
                        
                        activeParticles.emplace_back(ActiveParticle {
                            c, currAbsTimeMs, _particleLifeTimeMs, shapePoints.first, shapePoints.second, shapePoints.first,
                            float(rndSpread.getNextRandom() % 10001) / 10000.0f,
                            float(rndSpread.getNextRandom() % 10001) / 10000.0f,
                            float(rndSpread.getNextRandom() % 10001) / 10000.0f
                        });
                    }
                    
                    bornParticleCount = nextParticleCount;
                }
                
                for (auto ptc = activeParticles.begin(); ptc != activeParticles.end(); ++ptc) {
                    const float ptcAbsTimeMs = currAbsTimeMs - ptc->bornTimeMs;
                    const float lifeKoeff = ptcAbsTimeMs / ptc->lifeTimeMs;
                    MapElement &element = tmpMap[ptc->index * mapTextureWidth + i];

                    element.position = ptc->currentPosition;

                    if (ptc->currentPosition.x > _maxXYZ.x) _maxXYZ.x = ptc->currentPosition.x;
                    if (ptc->currentPosition.y > _maxXYZ.y) _maxXYZ.y = ptc->currentPosition.y;
                    if (ptc->currentPosition.z > _maxXYZ.z) _maxXYZ.z = ptc->currentPosition.z;
                    if (ptc->currentPosition.x < _minXYZ.x) _minXYZ.x = ptc->currentPosition.x;
                    if (ptc->currentPosition.y < _minXYZ.y) _minXYZ.y = ptc->currentPosition.y;
                    if (ptc->currentPosition.z < _minXYZ.z) _minXYZ.z = ptc->currentPosition.z;

                    
                    const math::vector3f direction = (ptc->end - ptc->start).normalized();
                    const float speed = _speedGraph.getValue(lifeKoeff, ptc->spreadKoeffSpeed);
                    ptc->currentPosition = ptc->currentPosition + direction * speed * (bakingFrameTimeMs / 1000.0f);
                    
                    // TODO: ptc position modificators
                    
                    
                    element.width = _widthGraph.getValue(lifeKoeff, ptc->spreadKoeffSize);
                    element.height = _heightGraph.getValue(lifeKoeff, ptc->spreadKoeffSize);
                    element.angle = 0.0f;
                    element.alpha = _alphaGraph.getValue(lifeKoeff, ptc->spreadKoeffAlpha);
                    element.mask = std::uint8_t(loop + 1); // Mask: 1 - newborn, 2 - old
                    element.history = std::uint8_t(std::min(float(core::MAX_PARTICLES_POSITION_HISTORY - 1), ptcAbsTimeMs / bakingFrameTimeMs));  // How old the particle is (f.e. 0 = just born, 254 = 254 * BakingTimeSec)
                    
                    if (lifeKoeff < std::numeric_limits<float>::epsilon()) { // mark where the particle starts
                        element.mask |= 128;
                    }
                }
                
            }
        }
        
        auto fract = [](float v) {
            return v - floor(v);
        };
        auto safeRange = [](float range) {
            return std::max(range, std::numeric_limits<float>::epsilon());
        };

        const math::vector3f positionRange = {safeRange(_maxXYZ.x - _minXYZ.x), safeRange(_maxXYZ.y - _minXYZ.y), safeRange(_maxXYZ.z - _minXYZ.z)};
        const math::vector2f sizeRange = {safeRange(_maxSize.x), safeRange(_maxSize.y)};

        for (std::uint32_t c = 0; c < mapVerticalCount; c++) {
            for (std::uint32_t i = 0; i < mapTextureWidth; i++) {
                const MapElement &element = tmpMap[c * mapTextureWidth + i];
                const std::size_t tvoff = c * core::VERTICAL_PIXELS_PER_PARTICLE;
                const math::vector3f positionKoeff = 255.0f * (element.position - _minXYZ) / positionRange;
                
                std::uint8_t *m0 = &_mapData[(tvoff + 0) * mapTextureWidth * 4 + i * 4];
                std::uint8_t *m1 = &_mapData[(tvoff + 1) * mapTextureWidth * 4 + i * 4];
                std::uint8_t *m2 = &_mapData[(tvoff + 2) * mapTextureWidth * 4 + i * 4];
                std::uint8_t *m3 = &_mapData[(tvoff + 3) * mapTextureWidth * 4 + i * 4];
                
                const float widthKoeff = 255.0f * element.width / sizeRange.x;
                const float heightKoeff = 255.0f * element.height / sizeRange.y;

                m0[0] = std::uint8_t(positionKoeff.x);                  // X hi
                m0[1] = std::uint8_t(255.0f * fract(positionKoeff.x));  // X low
                m0[2] = std::uint8_t(positionKoeff.y);                  // Y hi
                m0[3] = std::uint8_t(255.0f * fract(positionKoeff.y));  // Y low
                m1[0] = std::uint8_t(positionKoeff.z);                  // Z hi
                m1[1] = std::uint8_t(255.0f * fract(positionKoeff.z));  // Z low
                m1[2] = std::uint8_t(element.angle);                    // Angle hi
                m1[3] = std::uint8_t(255.0f * fract(element.angle));    // Angle low
                m2[0] = std::uint8_t(widthKoeff);                       // Width hi
                m2[1] = std::uint8_t(255.0f * fract(widthKoeff));       // Width low
                m2[2] = std::uint8_t(heightKoeff);                      // Height hi
                m2[3] = std::uint8_t(255.0f * fract(heightKoeff));      // Height low

                m3[0] = std::uint8_t(255.0f * element.alpha);           // Alpha
                m3[1] = 255;                                            // reserved
                m3[2] = element.history;
                m3[3] = element.mask;
            }
        }
        
//        for (std::size_t c = 0; c < _particlesToEmit * cyclesInRow; c++) {
//            for (std::size_t i = 0; i < mapTextureWidth; i++) {
//                std::uint8_t *m2 = &_mapData[(c * 3 + 2) * mapTextureWidth * 4 + i * 4];
//                printf("%02x ", int(m2[2]));
//            }
//            printf("\n");
//        }
//        printf("\n!!!\n");
        
        _mapWidth = mapTextureWidth;
        _mapHeight = mapTextureHeight;
        _mapTexture = nullptr;
        _description.setVector3f("minXYZ", _minXYZ);
        _description.setVector3f("maxXYZ", _maxXYZ);
        _description.setVector2f("maxSize", _maxSize);
    }

    std::pair<math::vector3f, math::vector3f> EmitterBakerImpl::_getShapePoints(float cycleOffset, const math::vector3f &shapeOffset) {
        switch (_shapeDistribution) {
            case ShapeDistribution::RANDOM:
            {
                const std::uint32_t r0 = _shapeGetRandom.getNextRandom();
                const std::uint32_t r1 = _shapeGetRandom.getNextRandom();
                return std::make_pair(_startShape.points[r0 % _startShape.points.size()], shapeOffset + _endShape.points[r1 % _endShape.points.size()]);
            }
            case ShapeDistribution::SHUFFLED:
            {
                const std::uint32_t r0 = _shapeGetRandom.getNextRandom();
                const std::size_t startIndex = r0 % _startShape.points.size();
                const std::size_t endIndex = std::size_t(float(startIndex) / float(_startShape.points.size()) * float(_endShape.points.size()));
                return std::make_pair(_startShape.points[startIndex], shapeOffset + _endShape.points[endIndex]);
            }
            case ShapeDistribution::LINEAR:
            {
                const std::size_t startIndex = std::size_t(cycleOffset * float(_startShape.points.size()));
                const std::size_t endIndex = std::size_t(cycleOffset * float(_endShape.points.size()));
                return std::make_pair(_startShape.points[startIndex], shapeOffset + _endShape.points[endIndex]);
            }
        }
        return {};
    }
    
    const foundation::RenderTexturePtr &EmitterBakerImpl::getMap(const foundation::RenderingInterfacePtr &rendering) {
        if (_mapTexture == nullptr && _mapData.empty() == false) {
            _mapTexture = rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, _mapWidth, _mapHeight, { _mapData.data() });
        }
        return _mapTexture;
    }
}

namespace core {
    EmitterBakerPtr makeEmitterBaker() {
        return std::make_shared<EmitterBakerImpl>();
    }
}
