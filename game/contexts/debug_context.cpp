
#include "debug_context.h"
#include <list>

namespace game {
    namespace {
        const std::size_t RND_SHAPE = 10000;
        const std::size_t RND_BORN_PARTICLE = 20000;
    
        std::size_t getNextRandom(std::size_t prevRandom) {
            return prevRandom * 6364136223846793005LL + 1442695040888963407LL;
        }
        std::size_t combineRandom(std::size_t r1, std::size_t r2) {
            return r1 * 6364136223846793005LL + r2 * 1442695040888963407LL;
        }
    }
    
    float Graph::getFilling(float t) {
        return std::min(std::max(t, 0.0f), 1.0f);
    }
    void Shape::generate(std::vector<math::vector3f> &points, const math::vector3f &offset, const math::vector3f &dir, std::size_t randomSeed, std::size_t amount) {
        if (type != Type::MESH) {
            points.clear();
            
            const float a = math::vector3f(0, 1, 0).angleTo(dir);
            const math::vector3f axis = math::vector3f(0, 1, 0).cross(dir).normalized();
            const math::transform3f rotation (axis, a);
            
            if (type == Type::DISK) {
                std::size_t random = randomSeed;

                for (std::size_t i = 0; i < amount; i++) {
                    random = getNextRandom(random);
                    const float koeff = float(i) / float(amount);
                    const float r = float(random % 100) / 200.0f * size;
                    const math::vector3f poff (r * std::cos(koeff * 2.0f * M_PI), 0, r * std::sin(koeff * 2.0f * M_PI));
                    points.emplace_back(offset + poff.transformed(rotation));
                }
            }
        }
    }
    
    Emitter::Emitter() {
        _ptcParams.additiveBlend = false;
        _ptcParams.orientation = voxel::ParticlesOrientation::CAMERA;
        _ptcParams.minXYZ = {-10, 0, -10};
        _ptcParams.maxXYZ = {10, 10, 10};
        _ptcParams.minMaxWidth = 1.0f;
        _ptcParams.minMaxHeight = 1.0f;
        _randomSeed = 0;
    }
    Emitter::~Emitter() {
    
    }
    
    void Emitter::setEndShapeOffset(const math::vector3f &offset) {
        _endShapeOffset = offset;
    }
    
    void Emitter::refresh(const foundation::RenderingInterfacePtr &rendering) {
        const math::vector3f emitterDir = _endShapeOffset.lengthSq() > std::numeric_limits<float>::epsilon() ? _endShapeOffset.normalized() : math::vector3f{0, 1, 0};
        const float maxShapeSize = std::max(_startShape.size, _endShape.size);

        _ptcParams.minXYZ = -1.0f * maxShapeSize * emitterDir.signs();
        _ptcParams.maxXYZ = _endShapeOffset + 1.0f * maxShapeSize * emitterDir.signs();
        
        if (_ptcParams.minXYZ.x > _ptcParams.maxXYZ.x) std::swap(_ptcParams.minXYZ.x, _ptcParams.maxXYZ.x);
        if (_ptcParams.minXYZ.y > _ptcParams.maxXYZ.y) std::swap(_ptcParams.minXYZ.y, _ptcParams.maxXYZ.y);
        if (_ptcParams.minXYZ.z > _ptcParams.maxXYZ.z) std::swap(_ptcParams.minXYZ.z, _ptcParams.maxXYZ.z);
        
        const std::size_t cycleLength = std::size_t(std::ceil(_emissionTimeMs / float(_bakingFrameTimeMs)));
        const std::size_t cyclesInRow = std::size_t(std::ceil((_isLooped ? std::max(_emissionTimeMs, _particleLifeTimeMs) : _emissionTimeMs + _particleLifeTimeMs) / _emissionTimeMs));
        
        _startShape.generate(_startShapePoints, {0, 0, 0}, emitterDir, combineRandom(_randomSeed, RND_SHAPE), 256);
        _endShape.generate(_endShapePoints, _endShapeOffset, emitterDir, combineRandom(_randomSeed, RND_SHAPE), 256);
        
        const std::uint32_t mapTextureWidth = std::uint32_t(cyclesInRow * cycleLength);
        const std::uint32_t mapTextureHeight = std::uint32_t(voxel::VERTICAL_PIXELS_PER_PARTICLE * _particlesToEmit * (_isLooped ? cyclesInRow : 1));
        
        _mapData.resize(mapTextureWidth * mapTextureHeight * 4);
        std::fill(_mapData.begin(), _mapData.end(), 128);
        
        std::list<ActiveParticle> activeParticles;
        std::size_t bornParticleCount = 0;

        // second loop is needed to bake old particles from previous cycle (_isLooped)
        for (std::size_t loop = 0; loop < std::size_t(_isLooped) + 1; loop++) { //
            std::size_t bornRandom = combineRandom(_randomSeed, RND_BORN_PARTICLE);

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
                        bornRandom = getNextRandom(bornRandom);
                        const std::pair<math::vector3f, math::vector3f> shapePoints = _getShapePoints(currEmissionTimeKoeff, bornRandom);
                        activeParticles.emplace_back(ActiveParticle {
                            c, bornRandom, currAbsTimeMs, _particleLifeTimeMs, shapePoints.first, shapePoints.second
                        });
                    }

                    bornParticleCount = nextParticleCount;
                }

                for (auto ptc = activeParticles.begin(); ptc != activeParticles.end(); ++ptc) {
                    const std::size_t voff = ptc->index * voxel::VERTICAL_PIXELS_PER_PARTICLE;
                    const float lifeKoeff = (currAbsTimeMs - ptc->bornTimeMs) / ptc->lifeTimeMs;

                    if (voff >= mapTextureHeight) {
                        printf("");
                    }
                    
                    std::uint8_t *m0 = &_mapData[(voff + 0) * mapTextureWidth * 4 + i * 4];
                    std::uint8_t *m1 = &_mapData[(voff + 1) * mapTextureWidth * 4 + i * 4];
                    std::uint8_t *m2 = &_mapData[(voff + 2) * mapTextureWidth * 4 + i * 4];

                    const math::vector3f direction = (ptc->end - ptc->start).normalized();
                    math::vector3f position = ptc->start + direction * _particleSpeed * lifeKoeff;

                    // TODO: ptc position modificators

                    float tmp;
                    const math::vector3f positionKoeff = 255.0f * (position - _ptcParams.minXYZ) / (_ptcParams.maxXYZ - _ptcParams.minXYZ);

                    m0[0] = std::uint8_t(positionKoeff.x);                            // X hi
                    m0[1] = std::uint8_t(255.0f * std::modf(positionKoeff.x, &tmp));  // X low
                    m0[2] = std::uint8_t(positionKoeff.y);                            // Y hi
                    m0[3] = std::uint8_t(255.0f * std::modf(positionKoeff.y, &tmp));  // Y low
                    m1[0] = std::uint8_t(positionKoeff.z);                            // X hi
                    m1[1] = std::uint8_t(255.0f * std::modf(positionKoeff.z, &tmp));  // X low
                    m1[2] = 0.0f; // Angle hi
                    m1[3] = 0.0f; // Angle low
                    m2[0] = 0;                                      // Width
                    m2[1] = 0;                                      // Height
                    m2[2] = 0;                                      // Angle
                    m2[3] = loop + 1;                               // Mask: 1 - newborn, 2 - old

                    if (lifeKoeff < std::numeric_limits<float>::epsilon()) { // mark where the particle starts
                        m2[3] |= 128;
                    }
                }
                
            }
        }
        
        for (std::size_t c = 0; c < _particlesToEmit * cyclesInRow; c++) {
            for (std::size_t i = 0; i < mapTextureWidth; i++) {
                std::uint8_t *m2 = &_mapData[(c * 3 + 2) * mapTextureWidth * 4 + i * 4];
                printf("%02x ", int(m2[3]));
            }
            printf("\n");
        }
        printf("\n!!!\n");

        _mapTexture = rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, mapTextureWidth, mapTextureHeight, { _mapData.data() });
        _ptcParams.looped = _isLooped;
        _ptcParams.secondsPerTextureWidth = float(mapTextureWidth) * (_bakingFrameTimeMs / 1000.0f);
    }

    foundation::RenderTexturePtr Emitter::getMap() const {
        return _mapTexture;
    }
    
    const voxel::ParticlesParams &Emitter::getParams() const {
        return _ptcParams;
    }

    std::pair<math::vector3f, math::vector3f> Emitter::_getShapePoints(float cycleOffset, std::size_t random) const {
        switch (_shapeDistribution) {
            case Shape::Distribution::RANDOM:
            {
                const std::size_t second = getNextRandom(random);
                return std::make_pair(_startShapePoints[random % _startShapePoints.size()], _endShapePoints[second % _endShapePoints.size()]);
            }
            case Shape::Distribution::SHUFFLED:
            {
                const std::size_t startIndex = random % _startShapePoints.size();
                const std::size_t endIndex = std::size_t(float(startIndex) / float(_startShapePoints.size()) * float(_endShapePoints.size()));
                return std::make_pair(_startShapePoints[startIndex], _endShapePoints[endIndex]);
            }
            case Shape::Distribution::LINEAR:
            {
                const std::size_t startIndex = std::size_t(cycleOffset * float(_startShapePoints.size()));
                const std::size_t endIndex = std::size_t(cycleOffset * float(_endShapePoints.size()));
                return std::make_pair(_startShapePoints[startIndex], _endShapePoints[endIndex]);
            }
        }
    }
    
}


namespace game {
    DebugContext::DebugContext(API &&api) : _api(std::move(api)) {
        _token = _api.platform->addPointerEventHandler(
            [this](const foundation::PlatformPointerEventArgs &args) -> bool {
                if (args.type == foundation::PlatformPointerEventArgs::EventType::START) {
                    _pointerId = args.pointerID;
                    _lockedCoordinates = { args.coordinateX, args.coordinateY };
                }
                if (args.type == foundation::PlatformPointerEventArgs::EventType::MOVE) {
                    if (_pointerId != foundation::INVALID_POINTER_ID) {
                        float dx = args.coordinateX - _lockedCoordinates.x;
                        float dy = args.coordinateY - _lockedCoordinates.y;

                        _orbit.xz = _orbit.xz.rotated(dx / 200.0f);

                        math::vector3f right = math::vector3f(0, 1, 0).cross(_orbit).normalized();
                        math::vector3f rotatedOrbit = _orbit.rotated(right, dy / 200.0f);

                        if (fabs(math::vector3f(0, 1, 0).dot(rotatedOrbit.normalized())) < 0.96f) {
                            _orbit = rotatedOrbit;
                        }

                        _lockedCoordinates = { args.coordinateX, args.coordinateY };
                    }
                }
                if (args.type == foundation::PlatformPointerEventArgs::EventType::FINISH) {
                    _timeSec = 0.0f;
                    _pointerId = foundation::INVALID_POINTER_ID;
                }
                if (args.type == foundation::PlatformPointerEventArgs::EventType::CANCEL) {
                    _pointerId = foundation::INVALID_POINTER_ID;
                }

                return true;
            }
        );
        
        _emitter.refresh(_api.rendering);
        
        _axis = _api.scene->addLineSet();
        _axis->setLine(0, {0, 0, 0}, {1000, 0, 0}, {1, 0, 0, 0.5});
        _axis->setLine(1, {0, 0, 0}, {0, 1000, 0}, {0, 1, 0, 0.5});
        _axis->setLine(2, {0, 0, 0}, {0, 0, 1000}, {0, 0, 1, 0.5});
        
        _api.resources->getOrLoadVoxelObject("objects/stool", [this](const std::vector<foundation::RenderDataPtr> &frames) {
            if (frames.size()) {
                _actor = _api.scene->addDynamicMesh(frames);
                _actor->setTransform(math::transform3f({0, 1, 0}, M_PI / 4).translated({20, 0, 40}));
            }
        });
        _api.resources->getOrLoadVoxelStatic("statics/ruins", [this](const foundation::RenderDataPtr &mesh) {
            if (mesh) {
                _thing = _api.scene->addStaticMesh(mesh);
            }
        });
        _api.resources->getOrLoadGround("grounds/white", [this](const foundation::RenderDataPtr &data, const foundation::RenderTexturePtr &texture) {
            if (data && texture) {
                _ground = _api.scene->addTexturedMesh(data, texture);
            }
        });
        _api.resources->getOrLoadTexture("textures/particles/test", [this](const foundation::RenderTexturePtr &texture){
            if (texture) {
//                std::uint32_t ptcparamssrc[] = {
//                    0x00000000,
//                    0x00000000,
//                    0x000000ff,
//                    0x00000000,
//                };
//                foundation::RenderTexturePtr ptcparams = _api.rendering->createTexture(foundation::RenderTextureFormat::RGBA8UN, 1, 4, { ptcparamssrc });
//                _ptc = _api.scene->addParticles(texture, ptcparams, voxel::ParticlesParams {
//                    .additiveBlend = false,
//                    .orientation = voxel::ParticlesOrientation::CAMERA,
//                    .minXYZ = {-5.0, 0, 0},
//                    .maxXYZ = {5.0, 0, 0},
//                    .minMaxWidth = {2.0f, 2.0f},
//                    .minMaxHeight = {2.0f, 2.0f},
//                });
                _ptc = _api.scene->addParticles(texture, _emitter.getMap(), _emitter.getParams());
                _ptc->setTransform(math::transform3f::identity().translated({32, 5, 32}));
            }
        });

        math::bound3f bb1 = {-0.5f, -0.5f, -0.5f, 63.0f + 0.5f, 19.0f + 0.5f, 63.0f + 0.5f};
        _bbox = _api.scene->addBoundingBox(bb1);
        _bbox->setColor({0.5f, 0.5f, 0.5f, 0.5f});
        
        
    }
    
    DebugContext::~DebugContext() {
        _api.platform->removeEventHandler(_token);
    }
    
    void DebugContext::update(float dtSec) {
        if (_ptc) {
            _ptc->setTime(_timeSec);
        }
        
        _api.scene->setCameraLookAt(_orbit + math::vector3f{32, 10, 32}, {32, 10, 32});
        _timeSec += dtSec;
    }
}
