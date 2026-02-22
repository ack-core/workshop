
#include "editor_particles_context.h"
#include <list>

namespace game {
    namespace {
        const std::uint64_t RND_SHAPE_FILL = 100;
        const std::uint64_t RND_SHAPE_GET = 200;
        const std::uint64_t RND_BORN_PARTICLE = 300;
        const std::uint64_t RND_GRAPH_SPREAD = 400;
    }
    
    Graph::Graph(float absMin, float absMax, float maxSpread, float defaultValue) : _absMin(absMin), _absMax(absMax), _maxSpread(maxSpread) {
        _points.emplace_back(Point { 0.0f, defaultValue, defaultValue });
        _maxVolume = defaultValue;
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

    float Shape::getMaxSize() const {
        if (type == Type::DISK) {
            return 2.0f * args.x;
        }
        else if (type == Type::BOX) {
            return args.max();
        }
        
        return 0.0f;
    }
    void Shape::generate(const voxel::SceneInterface::LineSetPtr &lineSet, const math::vector3f &dir, std::size_t randomSeed, std::size_t amount) {
        editor::RandomSource rnd = editor::RandomSource(randomSeed, RND_SHAPE_FILL);
        if (type == Type::DISK) {
            points.clear();
            std::size_t random = randomSeed;

            const float a = math::vector3f(0, 1, 0).angleTo(dir);
            const math::vector3f axis = math::vector3f(0, 1, 0).cross(dir.normalized()).normalized();
            const math::transform3f rotation (axis, -a);

            // points
            for (std::size_t i = 0; i < amount; i++) {
                const float koeff = 2.0f * M_PI * float(i) / float(amount);
                float r = 0.5f * args.x;
                
                if (fill) { // spread in [0..radius]
                    r = float(rnd.getNextRandom() % 991 + 10) / 2000.0f * args.x;
                }
                
                const math::vector3f poff (r * std::cosf(koeff), 0.0f, r * std::sinf(koeff));
                points.emplace_back(poff.transformed(rotation)); //
            }
            
            // lineset
            for (std::uint32_t i = 0; i < 36; i++) {
                const float koeff0 = 2.0f * M_PI * float(i) / 36.0f;
                const float koeff1 = 2.0f * M_PI * float(i + 1) / 36.0f;
                const math::vector3f p0 = math::vector3f(0.5f * args.x * std::cosf(koeff0), 0.0f, 0.5f * args.x * std::sinf(koeff0)).transformed(rotation);
                const math::vector3f p1 = math::vector3f(0.5f * args.x * std::cosf(koeff1), 0.0f, 0.5f * args.x * std::sinf(koeff1)).transformed(rotation);
                lineSet->setLine(i, p0, p1, {0.7f, 0.7f, 0.0f, 0.6f});
            }
            lineSet->capLineCount(36);
        }
        else if (type == Type::BOX) {
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
            
            // lineset
            const math::vector3f bbmin = -0.5f * args;
            const math::vector3f bbmax = 0.5f * args;
            const float alpha = 0.3f + 0.1 * float(std::max(dimCount, 1));
            lineSet->setLine(0,  math::vector3f(bbmin.x, bbmin.y, bbmin.z), math::vector3f(bbmax.x, bbmin.y, bbmin.z), {0.7f, 0.7f, 0.0f, alpha});
            lineSet->setLine(1,  math::vector3f(bbmin.x, bbmin.y, bbmin.z), math::vector3f(bbmin.x, bbmin.y, bbmax.z), {0.7f, 0.7f, 0.0f, alpha});
            lineSet->setLine(2,  math::vector3f(bbmax.x, bbmin.y, bbmax.z), math::vector3f(bbmax.x, bbmin.y, bbmin.z), {0.7f, 0.7f, 0.0f, alpha});
            lineSet->setLine(3,  math::vector3f(bbmax.x, bbmin.y, bbmax.z), math::vector3f(bbmin.x, bbmin.y, bbmax.z), {0.7f, 0.7f, 0.0f, alpha});
            lineSet->setLine(4,  math::vector3f(bbmin.x, bbmax.y, bbmin.z), math::vector3f(bbmax.x, bbmax.y, bbmin.z), {0.7f, 0.7f, 0.0f, alpha});
            lineSet->setLine(5,  math::vector3f(bbmin.x, bbmax.y, bbmin.z), math::vector3f(bbmin.x, bbmax.y, bbmax.z), {0.7f, 0.7f, 0.0f, alpha});
            lineSet->setLine(6,  math::vector3f(bbmax.x, bbmax.y, bbmax.z), math::vector3f(bbmax.x, bbmax.y, bbmin.z), {0.7f, 0.7f, 0.0f, alpha});
            lineSet->setLine(7,  math::vector3f(bbmax.x, bbmax.y, bbmax.z), math::vector3f(bbmin.x, bbmax.y, bbmax.z), {0.7f, 0.7f, 0.0f, alpha});
            lineSet->setLine(8,  math::vector3f(bbmin.x, bbmin.y, bbmin.z), math::vector3f(bbmin.x, bbmax.y, bbmin.z), {0.7f, 0.7f, 0.0f, alpha});
            lineSet->setLine(9,  math::vector3f(bbmax.x, bbmin.y, bbmin.z), math::vector3f(bbmax.x, bbmax.y, bbmin.z), {0.7f, 0.7f, 0.0f, alpha});
            lineSet->setLine(10, math::vector3f(bbmax.x, bbmin.y, bbmax.z), math::vector3f(bbmax.x, bbmax.y, bbmax.z), {0.7f, 0.7f, 0.0f, alpha});
            lineSet->setLine(11, math::vector3f(bbmin.x, bbmin.y, bbmax.z), math::vector3f(bbmin.x, bbmax.y, bbmax.z), {0.7f, 0.7f, 0.0f, alpha});

            lineSet->capLineCount(12);
        }
    }
    
    Emitter::Emitter() {
        _ptcParams.additiveBlend = false;
        _ptcParams.orientation = voxel::ParticlesParams::ParticlesOrientation::CAMERA;
        _ptcParams.minXYZ = {-10, 0, -10};
        _ptcParams.maxXYZ = {10, 10, 10};
        _ptcParams.maxSize = {1.0f, 1.0f};
        _randomSeed = 0;
    }
    Emitter::~Emitter() {
    
    }
    
    void Emitter::setParameters(const util::Description &params) {
        std::size_t bakingTimeTable[] = {
            0, 10, 20, 50, 100
        };
        
        _emissionTimeMs = params.getInteger("emissionTimeMs", 0);
        _particleLifeTimeMs = params.getInteger("particleLifeTimeMs", 0);
        _bakingFrameTimeMs = bakingTimeTable[params.getInteger("bakingFrameType", 0)];
        _particlesToEmit = params.getInteger("particlesToEmit", 0);
        _isLooped = params.getBool("looped", false);
        _randomSeed = params.getInteger("randomSeed", 0);
        _startShape.type = static_cast<Shape::Type>(params.getInteger("startShapeType", 0));
        _startShape.fill = params.getBool("startShapeFill", false);
        _startShape.args = params.getVector3f("startShapeArgs", {});
        _endShape.type = static_cast<Shape::Type>(params.getInteger("endShapeType", 0));
        _endShape.fill = params.getBool("endShapeFill", false);
        _endShape.args = params.getVector3f("endShapeArgs", {});
        _endShapeOffset = params.getVector3f("endShapeOffset", {});
        _shapeDistribution = static_cast<ShapeDistribution>(params.getInteger("shapeDistributionType", 0));
        _ptcParams.orientation = static_cast<voxel::ParticlesParams::ParticlesOrientation>(params.getInteger("particleOrientation", 0));
        _ptcParams.additiveBlend = params.getBool("additiveBlend", false);
        _emissionGraph.setPointsFromString(params.getString("emissionGraphData", ""));
        _widthGraph.setPointsFromString(params.getString("widthGraphData", ""));
        _heightGraph.setPointsFromString(params.getString("heightGraphData", ""));
        _speedGraph.setPointsFromString(params.getString("speedGraphData", ""));
        _alphaGraph.setPointsFromString(params.getString("alphaGraphData", ""));
    }
    
    void Emitter::setEndShapeOffset(const math::vector3f &offset) {
        _endShapeOffset = offset;
    }
    
    void Emitter::refresh(const foundation::RenderingInterfacePtr &rendering, const voxel::SceneInterface::LineSetPtr &shapeStart, const voxel::SceneInterface::LineSetPtr &shapeEnd) {
        _shapeGetRandom = editor::RandomSource(_randomSeed, RND_SHAPE_GET);
        
        const math::vector3f emitterDir = _endShapeOffset.lengthSq() > 0.1f ? _endShapeOffset.normalized() : math::vector3f{0, 1, 0};
        const std::size_t cycleLength = std::size_t(std::ceil(_emissionTimeMs / float(_bakingFrameTimeMs)));
        const std::size_t cyclesInRow = std::size_t(std::ceil((_isLooped ? std::max(_emissionTimeMs, _particleLifeTimeMs) : _emissionTimeMs + _particleLifeTimeMs) / _emissionTimeMs));
        
        _startShape.generate(shapeStart, emitterDir.normalized(), _randomSeed, 1024);
        _endShape.generate(shapeEnd, emitterDir.normalized(), _randomSeed, 1024);
        
        const std::uint32_t mapTextureWidth = std::uint32_t(cyclesInRow * cycleLength);
        const std::uint32_t mapVerticalCount = std::uint32_t(_particlesToEmit * (_isLooped ? cyclesInRow : 1));
        const std::uint32_t mapTextureHeight = voxel::VERTICAL_PIXELS_PER_PARTICLE * mapVerticalCount;
        
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
        
        _ptcParams.minXYZ = {0, 0, 0};
        _ptcParams.maxXYZ = {0, 0, 0};
        _ptcParams.maxSize = {_widthGraph.getMaxValue(), _heightGraph.getMaxValue()};

        editor::RandomSource rndSpread = editor::RandomSource(RND_GRAPH_SPREAD, _randomSeed);
        
        // second loop is needed to bake old particles from previous cycle (_isLooped)
        for (std::size_t loop = 0; loop < std::size_t(_isLooped) + 1; loop++) { //
            for (std::size_t i = 0; i < mapTextureWidth; i++) {
                const std::size_t timeIndex = i + loop * mapTextureWidth;
                const float currAbsTimeMs = float((timeIndex + 0) * _bakingFrameTimeMs);
                const float nextAbsTimeMs = float((timeIndex + 1) * _bakingFrameTimeMs);
                const float currEmissionTimeKoeff = float(((timeIndex % cycleLength) + 0) * _bakingFrameTimeMs) / float(_emissionTimeMs);
                const float nextEmissionTimeKoeff = float(((timeIndex % cycleLength) + 1) * _bakingFrameTimeMs) / float(_emissionTimeMs);
                                
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

                    if (ptc->currentPosition.x > _ptcParams.maxXYZ.x) _ptcParams.maxXYZ.x = ptc->currentPosition.x;
                    if (ptc->currentPosition.y > _ptcParams.maxXYZ.y) _ptcParams.maxXYZ.y = ptc->currentPosition.y;
                    if (ptc->currentPosition.z > _ptcParams.maxXYZ.z) _ptcParams.maxXYZ.z = ptc->currentPosition.z;
                    if (ptc->currentPosition.x < _ptcParams.minXYZ.x) _ptcParams.minXYZ.x = ptc->currentPosition.x;
                    if (ptc->currentPosition.y < _ptcParams.minXYZ.y) _ptcParams.minXYZ.y = ptc->currentPosition.y;
                    if (ptc->currentPosition.z < _ptcParams.minXYZ.z) _ptcParams.minXYZ.z = ptc->currentPosition.z;

                    
                    const math::vector3f direction = (ptc->end - ptc->start).normalized();
                    const float speed = _speedGraph.getValue(lifeKoeff, ptc->spreadKoeffSpeed);
                    ptc->currentPosition = ptc->currentPosition + direction * speed * (_bakingFrameTimeMs / 1000.0f);
                    
                    // TODO: ptc position modificators
                    
                    
                    element.width = _widthGraph.getValue(lifeKoeff, ptc->spreadKoeffSize);
                    element.height = _heightGraph.getValue(lifeKoeff, ptc->spreadKoeffSize);
                    element.angle = 0.0f;
                    element.alpha = _alphaGraph.getValue(lifeKoeff, ptc->spreadKoeffAlpha);
                    element.mask = loop + 1; // Mask: 1 - newborn, 2 - old
                    element.history = std::uint8_t(std::min(255.0f, ptcAbsTimeMs / _bakingFrameTimeMs));  // How old the particle is (f.e. 0 = just born, 255 = 255 * BakingTimeSec)
                    
                    if (lifeKoeff < std::numeric_limits<float>::epsilon()) { // mark where the particle starts
                        element.mask |= 128;
                    }
                }
                
            }
        }
        
        auto fract = [](float v) {
            return v - floor(v);
        };
        
        for (std::uint32_t c = 0; c < mapVerticalCount; c++) {
            for (std::uint32_t i = 0; i < mapTextureWidth; i++) {
                const MapElement &element = tmpMap[c * mapTextureWidth + i];
                const std::size_t tvoff = c * voxel::VERTICAL_PIXELS_PER_PARTICLE;
                const math::vector3f positionKoeff = 255.0f * (element.position - _ptcParams.minXYZ) / (_ptcParams.maxXYZ - _ptcParams.minXYZ);
                
                std::uint8_t *m0 = &_mapData[(tvoff + 0) * mapTextureWidth * 4 + i * 4];
                std::uint8_t *m1 = &_mapData[(tvoff + 1) * mapTextureWidth * 4 + i * 4];
                std::uint8_t *m2 = &_mapData[(tvoff + 2) * mapTextureWidth * 4 + i * 4];
                std::uint8_t *m3 = &_mapData[(tvoff + 3) * mapTextureWidth * 4 + i * 4];
                
                const float widthKoeff = 255.0f * element.width / _ptcParams.maxSize.x;
                const float heightKoeff = 255.0f * element.height / _ptcParams.maxSize.y;

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

                m3[0] = 255.0f * element.alpha;                         // Alpha
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
        
        _mapTexture = rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, mapTextureWidth, mapTextureHeight, { _mapData.data() });
        _ptcParams.bakingTimeSec = float(_bakingFrameTimeMs) / 1000.0f;
    }

    const foundation::RenderTexturePtr &Emitter::getMap() const {
        return _mapTexture;
    }
    const std::uint8_t * Emitter::getMapRaw() const {
        return _mapData.data();
    }
    const voxel::ParticlesParams &Emitter::getParams() const {
        return _ptcParams;
    }

    std::pair<math::vector3f, math::vector3f> Emitter::_getShapePoints(float cycleOffset, const math::vector3f &shapeOffset) {
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
    }
    
}


namespace game {
    void EditorNodeParticles::update(float dtSec) {
        if (particles) {
            globalPosition = localPosition + (parent ? parent->globalPosition : math::vector3f(0, 0, 0));
            particles->setTransform(math::transform3f::identity().translated(globalPosition));
            particles->setTime(currentTime, 0.0f);
            currentTime += dtSec;
        }
    }
    void EditorNodeParticles::setResourcePath(const API &api, const std::string &path) {
        if (path.empty()) {
            resourcePath = EDITOR_EMPTY_RESOURCE_PATH;
            particles = nullptr;
            texture = nullptr;
            map = nullptr;
            originDesc = {};
        }
        else {
            resourcePath = path;
            api.resources->getOrLoadEmitter(path.c_str(), [this, &api](const util::Description &desc, const foundation::RenderTexturePtr &m, const foundation::RenderTexturePtr &t) {
                if (desc.empty() == false) {
                    originDesc = desc;
                    texture = t;
                    map = m;
                    
                    if (map) {
                        voxel::ParticlesParams parameters (*originDesc);
                        particles = api.scene->addParticles(texture, map, parameters);
                        api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
                    }
                }
            });
        }
    }

    EditorParticlesContext::EditorParticlesContext(API &&api, NodeAccessInterface &nodeAccess, CameraAccessInterface &cameraAccess)
    : _api(std::move(api))
    , _nodeAccess(nodeAccess)
    , _cameraAccess(cameraAccess)
    {
        EditorNode::makeByType[std::size_t(voxel::WorldInterface::NodeType::PARTICLES)] = [](std::size_t typeIndex) {
            return std::static_pointer_cast<EditorNode>(std::make_shared<EditorNodeParticles>(typeIndex));
        };
        
        _handlers["editor.selectNode"] = &EditorParticlesContext::_selectNode;
        _handlers["editor.clearNodeSelection"] = &EditorParticlesContext::_clearNodeSelection;
        _handlers["editor.setPath"] = &EditorParticlesContext::_setResourcePath;
        _handlers["editor.startEditing"] = &EditorParticlesContext::_startEditing;
        _handlers["editor.stopEditing"] = &EditorParticlesContext::_stopEditing;
        _handlers["editor.reloadResources"] = &EditorParticlesContext::_reload;
        _handlers["editor.particles.emission"] = &EditorParticlesContext::_emissionSet;
        _handlers["editor.particles.visual"] = &EditorParticlesContext::_visualSet;
        _handlers["editor.particles.emissionGraph"] = &EditorParticlesContext::_emissionGraphSet;
        _handlers["editor.particles.widthGraph"] = &EditorParticlesContext::_widthGraphSet;
        _handlers["editor.particles.heightGraph"] = &EditorParticlesContext::_heightGraphSet;
        _handlers["editor.particles.speedGraph"] = &EditorParticlesContext::_speedGraphSet;
        _handlers["editor.particles.alphaGraph"] = &EditorParticlesContext::_alphaGraphSet;
        _handlers["editor.resource.save"] = &EditorParticlesContext::_save;

        _editorEventsToken = _api.platform->addEditorEventHandler([this](const std::string &msg, const std::string &data) {
            auto handler = _handlers[msg];
            if (handler) {
                return (this->*handler)(data);
            }
            return false;
        });
    }
    
    EditorParticlesContext::~EditorParticlesContext() {
        _api.platform->removeEventHandler(_editorEventsToken);
    }
    
    void EditorParticlesContext::update(float dtSec) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            if (node->particles) {
                math::vector3f position = node->globalPosition;
                
                if (_endShapeTool && node->currentDesc.has_value()) {
                    _endShapeTool->setBase(position);
                    _shapeStartLineset->setPosition(position);
                    _shapeEndLineset->setPosition(position + _endShapeTool->getPosition());
                    _shapeConnectLineset->setPosition(position);
                    _shapeConnectLineset->setLine(0, {0, 0, 0}, _endShapeTool->getPosition(), {0.7f, 0.7f, 0.0f, 0.6f}, false);
                }
            }
        }
    }
    
    void EditorParticlesContext::_clearEditingTools() {
        _endShapeTool = nullptr;
        _shapeConnectLineset = nullptr;
        _shapeStartLineset = nullptr;
        _shapeEndLineset = nullptr;
    }
    
    void EditorParticlesContext::_endShapeDragFinished() {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            math::vector3f off = _endShapeTool->getPosition();
            node->currentDesc->setVector3f("endShapeOffset", off);
            node->emitter.setEndShapeOffset(off);
            node->emitter.refresh(_api.rendering, _shapeStartLineset, _shapeEndLineset);
            _recreateParticles(*node, true);
            _api.platform->sendEditorMsg("engine.ptcEndOffset", util::strstream::ftos(off.x) + " " + util::strstream::ftos(off.y) + " " + util::strstream::ftos(off.z));
        }
    }

    void EditorParticlesContext::_recreateParticles(EditorNodeParticles &node, bool editing) {
        if (editing) {
            node.particles = _api.scene->addParticles(node.texture, node.emitter.getMap(), node.emitter.getParams());
        }
        else {
            voxel::ParticlesParams parameters (*node.originDesc);
            node.particles = _api.scene->addParticles(node.texture, node.map, parameters);
        }
    }

    bool EditorParticlesContext::_selectNode(const std::string &data) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            _api.platform->sendEditorMsg("engine.nodeSelected", data + " inspect_particles " + node->resourcePath);
        }
        else {
            _clearEditingTools();
        }
        return false;
    }
    
    bool EditorParticlesContext::_clearNodeSelection(const std::string &data) {
        _clearEditingTools();
        return false;
    }

    bool EditorParticlesContext::_setResourcePath(const std::string &data) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            node->setResourcePath(_api, data);
            return true;
        }
        return false;
    }
    
    bool EditorParticlesContext::_startEditing(const std::string &data) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            if (node->originDesc.has_value()) {
                node->currentDesc = node->originDesc;
                node->endShapeOffset = node->currentDesc->getVector3f("endShapeOffset", {});
                
                _endShapeTool = std::make_unique<MovingTool>(_api, _cameraAccess, node->endShapeOffset, 0.4f);
                _endShapeTool->setOnDragEnd([this] {
                    this->_endShapeDragFinished();
                });
                _shapeConnectLineset = _api.scene->addLineSet();
                _shapeStartLineset = _api.scene->addLineSet();
                _shapeEndLineset = _api.scene->addLineSet();

                node->emitter.setParameters(node->currentDesc.value());
                node->emitter.setEndShapeOffset(_endShapeTool->getPosition());
                node->emitter.refresh(_api.rendering, _shapeStartLineset, _shapeEndLineset);
                _recreateParticles(*node, true);
                
                std::string args = util::Description::serialize(*node->currentDesc);
                _api.platform->sendEditorMsg("engine.particles.editing", args);
                return true;
            }
        }
        return false;
    }
    
    bool EditorParticlesContext::_stopEditing(const std::string &data) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            if (node->currentDesc) {
                node->currentDesc.reset();
                
                if (node->map) {
                    _recreateParticles(*node, false);
                }
            }
            if (node->originDesc) {
                node->endShapeOffset = node->originDesc->getVector3f("endShapeOffset", {});
            }
        }
        _clearEditingTools();
        return false;
    }
    
    bool EditorParticlesContext::_reload(const std::string &data) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            _api.resources->removeEmitter(node->resourcePath.data());
            _api.resources->removeEmitter(data.c_str());

            // updating all emitter nodes
            _nodeAccess.forEachNode([this](const std::shared_ptr<EditorNode> &node) {
                std::shared_ptr<EditorNodeParticles> emitterNode = std::dynamic_pointer_cast<EditorNodeParticles>(node);
                if (emitterNode && emitterNode->particles) {
                    const char *path = emitterNode->resourcePath.c_str();
                    _api.resources->getOrLoadEmitter(path, [emitterNode, this](const util::Description &desc, const foundation::RenderTexturePtr &m, const foundation::RenderTexturePtr &t) {
                        if (desc.empty() == false) {
                            emitterNode->originDesc = desc;
                            emitterNode->texture = t;
                            emitterNode->map = m;

                            if (m && t) {
                                _recreateParticles(*emitterNode, false);
                                _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
                            }
                        }
                    });
                }
            });
            
            _api.platform->sendEditorMsg("engine.resourcesReloaded", "");
            return true;
        }
        return false;
    }
    
    bool EditorParticlesContext::_save(const std::string &data) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            _savingCfg = util::Description::serialize(*node->currentDesc);
            _api.platform->saveFile((node->resourcePath + ".txt").c_str(), reinterpret_cast<const std::uint8_t *>(_savingCfg.data()), _savingCfg.length(), [this, node](bool result) {
                const std::size_t width = node->emitter.getMap()->getWidth();
                const std::size_t height = node->emitter.getMap()->getHeight();
                auto tgaResult = editor::writeTGA(node->emitter.getMapRaw(), width, height);
                _savingMap = std::move(tgaResult.first);
            
                _api.platform->saveFile((node->resourcePath + ".tga").c_str(), _savingMap.get(), tgaResult.second, [this, path = node->resourcePath](bool result) {
                    _api.platform->sendEditorMsg("engine.saved", path);
                    _savingMap = nullptr;
                });
            });

            _clearEditingTools();
            node->currentDesc.reset();

            return true;
        }
        return false;
    }

    bool EditorParticlesContext::_emissionSet(const std::string &data) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            util::strstream input(data.c_str(), data.length());
            int emissionTimeMs, particleLifeTimeMs, bakingFrameType, particlesToEmit, rndSeed;
            bool looped;
            
            if (input >> emissionTimeMs >> particleLifeTimeMs >> bakingFrameType >> particlesToEmit >> looped >> rndSeed) {
                util::Description &desc = node->currentDesc.value();
                desc.setBool("looped", looped);
                desc.setInteger("emissionTimeMs", emissionTimeMs);
                desc.setInteger("particleLifeTimeMs", particleLifeTimeMs);
                desc.setInteger("bakingFrameType", bakingFrameType);
                desc.setInteger("particlesToEmit", particlesToEmit);
                desc.setInteger("randomSeed", rndSeed);
                node->emitter.setParameters(*node->currentDesc);
                node->emitter.refresh(_api.rendering, _shapeStartLineset, _shapeEndLineset);
                desc.setVector3f("minXYZ", node->emitter.getParams().minXYZ);
                desc.setVector3f("maxXYZ", node->emitter.getParams().maxXYZ);
                desc.setVector2f("maxSize", node->emitter.getParams().maxSize);
                _recreateParticles(*node, true);
            }
            return true;
        }
        return false;
    }
    bool EditorParticlesContext::_visualSet(const std::string &data) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            util::strstream input(data.c_str(), data.length());
            math::vector3f startShapeArgs, endShapeArgs, endShapeOffset;
            int startShapeType, endShapeType, shapeDistributionType, particleOrientation;
            bool startShapeFill, endShapeFill;
            bool success = false;

            success |= input >> startShapeType >> startShapeFill;
            success |= input >> startShapeArgs.x >> startShapeArgs.y >> startShapeArgs.z;
            success |= input >> endShapeType >> endShapeFill;
            success |= input >> endShapeArgs.x >> endShapeArgs.y >> endShapeArgs.z;
            success |= input >> endShapeOffset.x >> endShapeOffset.y >> endShapeOffset.z;
            success |= input >> shapeDistributionType >> particleOrientation;
            
            if (success) {
                util::Description &desc = node->currentDesc.value();
                desc.setInteger("startShapeType", startShapeType);
                desc.setBool("startShapeFill", startShapeFill);
                desc.setVector3f("startShapeArgs", startShapeArgs);
                desc.setInteger("endShapeType", endShapeType);
                desc.setBool("endShapeFill", endShapeFill);
                desc.setVector3f("endShapeArgs", endShapeArgs);
                desc.setVector3f("endShapeOffset", endShapeOffset);
                desc.setInteger("shapeDistributionType", shapeDistributionType);
                desc.setInteger("particleOrientation", particleOrientation);
                node->endShapeOffset = endShapeOffset;
                node->emitter.setParameters(*node->currentDesc);
                node->emitter.refresh(_api.rendering, _shapeStartLineset, _shapeEndLineset);
                desc.setVector3f("minXYZ", node->emitter.getParams().minXYZ);
                desc.setVector3f("maxXYZ", node->emitter.getParams().maxXYZ);
                desc.setVector2f("maxSize", node->emitter.getParams().maxSize);
                _recreateParticles(*node, true);
            }
            return true;
        }
        return false;
    }
    bool EditorParticlesContext::_graphSet(const std::string &data, const std::string &name) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            util::Description &desc = node->currentDesc.value();
            desc.setString(name.c_str(), data);
            node->emitter.setParameters(*node->currentDesc);
            node->emitter.refresh(_api.rendering, _shapeStartLineset, _shapeEndLineset);
            desc.setVector3f("minXYZ", node->emitter.getParams().minXYZ);
            desc.setVector3f("maxXYZ", node->emitter.getParams().maxXYZ);
            desc.setVector2f("maxSize", node->emitter.getParams().maxSize);
            _recreateParticles(*node, true);
            return true;
        }
        return false;
    }
    bool EditorParticlesContext::_emissionGraphSet(const std::string &data) {
        return _graphSet(data, "emissionGraphData");
    }
    bool EditorParticlesContext::_widthGraphSet(const std::string &data) {
        return _graphSet(data, "widthGraphData");
    }
    bool EditorParticlesContext::_heightGraphSet(const std::string &data) {
        return _graphSet(data, "heightGraphData");
    }
    bool EditorParticlesContext::_speedGraphSet(const std::string &data) {
        return _graphSet(data, "speedGraphData");
    }
    bool EditorParticlesContext::_alphaGraphSet(const std::string &data) {
        return _graphSet(data, "alphaGraphData");
    }

}


