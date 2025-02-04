
#include "debug_context.h"
#include <list>

namespace game {
    namespace {
        const std::size_t RND_SHAPE = 0;
        const std::size_t RND_BORN_PARTICLE = 0;
    }

    float Graph::getFilling(float t) {
        return std::min(std::max(t, 0.0f), 1.0f);
    }
    void Shape::generate(std::size_t randomSeed, std::size_t amount) {
        points.clear();
        
    }

    Emitter::Emitter() {
        _ptcParams.additiveBlend = false;
        _ptcParams.orientation = voxel::ParticlesOrientation::CAMERA;
        _ptcParams.minXYZ = {-0.5, 0, -0.5};
        _ptcParams.maxXYZ = {0.5, 10, 0.5};
        _ptcParams.minMaxWidth = 1.0f;
        _ptcParams.minMaxHeight = 1.0f;
    }
    Emitter::~Emitter() {
    
    }
    
    void Emitter::setEndShapeOffset(const math::vector3f &offset) {
        _endShapeOffset = offset;
    }
    
    void Emitter::refresh(const foundation::RenderingInterfacePtr &rendering) {
        // TODO: recalculate bbmin/bbmax
        
        _startShape.generate(_randomSeed ^ RND_SHAPE, _particlesToEmit);
        _endShape.generate(_randomSeed ^ RND_SHAPE, _particlesToEmit);

        const std::size_t cycleLength = std::size_t(std::ceil(_emissionTimeMs / float(_bakingFrameTimeMs)));
        const std::size_t cyclesInRow = std::size_t(std::ceil((_emissionTimeMs + _particleLifeTimeMs) / _emissionTimeMs));
        
        const std::uint32_t mapTextureWidth = std::uint32_t(cyclesInRow * cycleLength);
        const std::uint32_t mapTextureHeight = std::uint32_t(voxel::VERTICAL_PIXELS_PER_PARTICLE * _particlesToEmit * (_isLooped ? cyclesInRow : 1));
        
        _mapData.resize(mapTextureWidth * mapTextureHeight * 4);
        std::fill(_mapData.begin(), _mapData.end(), 0);
        
        std::list<ActiveParticle> activeParticles;
        std::size_t bornParticleCount = 0;
        std::size_t bornRandom = _randomSeed ^ RND_BORN_PARTICLE;

        // TODO: create particles from prev cycles (_isLooped)
        
        for (std::size_t i = 0; i < mapTextureWidth; i++) {
            const float currAbsTimeMs = float((i + 0) * _bakingFrameTimeMs);
            const float nextAbsTimeMs = float((i + 1) * _bakingFrameTimeMs);
            const float currEmissionTimeKoeff = float(((i % cycleLength) + 0) * _bakingFrameTimeMs) / float(_emissionTimeMs);
            const float nextEmissionTimeKoeff = float(((i % cycleLength) + 1) * _bakingFrameTimeMs) / float(_emissionTimeMs);
            
            float emissinGraphFilling = _emissionGraph.getFilling(nextEmissionTimeKoeff);
            const std::size_t nextEmissionGraphValue = std::size_t(std::ceil(float(_particlesToEmit) * emissinGraphFilling));
            const std::size_t nextParticleCount = std::min(i / cycleLength * _particlesToEmit + nextEmissionGraphValue, _isLooped ? std::size_t(-1) : _particlesToEmit);
            
            for (auto ptc = activeParticles.begin(); ptc != activeParticles.end(); ) {
                if (nextAbsTimeMs - ptc->bornTimeMs > ptc->lifeTimeMs) {
                    ptc = activeParticles.erase(ptc);
                }
                else {
                    ++ptc;
                }
            }

            for (std::size_t c = bornParticleCount; c < nextParticleCount; c++) {
                bornRandom = _nextRandom(bornRandom);
                const std::pair<math::vector3f, math::vector3f> shapePoints = _getShapePoints(currEmissionTimeKoeff, bornRandom);
                activeParticles.emplace_back(ActiveParticle {
                    c, bornRandom, currAbsTimeMs, _particleLifeTimeMs, shapePoints.first, shapePoints.second
                });
            }

            for (auto ptc = activeParticles.begin(); ptc != activeParticles.end(); ++ptc) {
                const std::size_t voff = ptc->index * voxel::VERTICAL_PIXELS_PER_PARTICLE;
                const float lifeKoeff = (currAbsTimeMs - ptc->bornTimeMs) / ptc->lifeTimeMs;

                std::uint8_t *m0 = &_mapData[(voff + 0) * mapTextureWidth * 4 + i * 4];
                std::uint8_t *m1 = &_mapData[(voff + 1) * mapTextureWidth * 4 + i * 4];

                math::vector3f direction = (ptc->end - ptc->start).normalized();
                math::vector3f position = ptc->start + direction * _particleSpeed * lifeKoeff;

                // TODO: ptc position modificators

                const math::vector3f positionKoeff = (position - _ptcParams.minXYZ) / (_ptcParams.maxXYZ - _ptcParams.minXYZ);

                m0[0] = std::uint8_t(positionKoeff.x * 255.0f); // X
                m0[1] = std::uint8_t(positionKoeff.y * 255.0f); // Y
                m0[2] = std::uint8_t(positionKoeff.z * 255.0f); // Z
                m0[3] = 0;                                      // History
                m1[0] = 0;                                      // Width
                m1[1] = 0;                                      // Height
                m1[2] = 0;                                      // Angle
                m1[3] = 0xff;                                   // isAlive

            }
            
            bornParticleCount = nextParticleCount;
        }
        
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
    
    std::size_t Emitter::_nextRandom(std::size_t prev) {
        return 0;
    }

    std::pair<math::vector3f, math::vector3f> Emitter::_getShapePoints(float cycleOffset, std::size_t random) const {
        return std::make_pair(math::vector3f{0, 0, 0}, _endShapeOffset);
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
