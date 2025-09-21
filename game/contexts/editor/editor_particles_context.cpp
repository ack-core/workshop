
#include "editor_particles_context.h"
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
        if (type == Type::DISK) {
            points.clear();
            std::size_t random = randomSeed;

            const float a = math::vector3f(0, 1, 0).angleTo(dir);
            const math::vector3f axis = math::vector3f(0, 1, 0).cross(dir.normalized()).normalized();
            const math::transform3f rotation (axis, -a);

            // points
            for (std::size_t i = 0; i < amount; i++) {
                const float koeff = 2.0f * M_PI * float(i) / float(amount);
                float r = args.x;
                
                if (fill) { // spread in [0..radius]
                    random = getNextRandom(random);
                    r = float(random % 991 + 10) / 1000.0f * args.x;
                }
                
                const math::vector3f poff (r * std::cosf(koeff), 0.0f, r * std::sinf(koeff));
                points.emplace_back(poff.transformed(rotation)); //
            }
            
            // lineset
            for (std::uint32_t i = 0; i < 36; i++) {
                const float koeff0 = 2.0f * M_PI * float(i) / 36.0f;
                const float koeff1 = 2.0f * M_PI * float(i + 1) / 36.0f;
                const math::vector3f p0 = math::vector3f(args.x * std::cosf(koeff0), 0.0f, args.x * std::sinf(koeff0)).transformed(rotation);
                const math::vector3f p1 = math::vector3f(args.x * std::cosf(koeff1), 0.0f, args.x * std::sinf(koeff1)).transformed(rotation);
                lineSet->setLine(i, p0, p1, {0.4f, 0.4f, 0.0f, 0.1f});
            }
        }
        else if (type == Type::BOX) {
            points.clear();
            std::size_t random = randomSeed;

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
                        random = getNextRandom(random);
                        koeff.x = float(random % 10001) / float(10000) - 0.5f;
                        random = getNextRandom(random);
                        koeff.y = float(random % 10001) / float(10000) - 0.5f;
                        random = getNextRandom(random);
                        koeff.z = float(random % 10001) / float(10000) - 0.5f;
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
                        random = getNextRandom(random);
                        koeff.x = float(random % 10001) / float(10000) - 0.5f;
                        random = getNextRandom(random);
                        koeff.y = float(random % 10001) / float(10000) - 0.5f;
                        random = getNextRandom(random);
                        koeff.z = float(random % 10001) / float(10000) - 0.5f;
                        
                        int dimToAlign = dimIndexes[i % dimCount];
                        random = getNextRandom(random);
                        koeff.flat[dimToAlign] = random % 1000 >= 500 ? 0.5f : -0.5f;
                        
                        points.emplace_back(args * koeff);
                    }
                }

            }
            
            // lineset
            const math::vector3f bbmin = -0.5f * args;
            const math::vector3f bbmax = 0.5f * args;
            
            lineSet->setLine(0, math::vector3f(bbmin.x, bbmin.y, bbmin.z), math::vector3f(bbmax.x, bbmin.y, bbmin.z), {0.4f, 0.4f, 0.0f, 0.1f});
            lineSet->setLine(1, math::vector3f(bbmin.x, bbmin.y, bbmin.z), math::vector3f(bbmin.x, bbmin.y, bbmax.z), {0.4f, 0.4f, 0.0f, 0.1f});
            lineSet->setLine(2, math::vector3f(bbmax.x, bbmin.y, bbmax.z), math::vector3f(bbmax.x, bbmin.y, bbmin.z), {0.4f, 0.4f, 0.0f, 0.1f});
            lineSet->setLine(3, math::vector3f(bbmax.x, bbmin.y, bbmax.z), math::vector3f(bbmin.x, bbmin.y, bbmax.z), {0.4f, 0.4f, 0.0f, 0.1f});
        }
    }
    
    Emitter::Emitter() {
        _ptcParams.additiveBlend = false;
        _ptcParams.orientation = layouts::ParticlesParams::ParticlesOrientation::CAMERA;
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
    
    void Emitter::refresh(const foundation::RenderingInterfacePtr &rendering, const voxel::SceneInterface::LineSetPtr &shapeStart, const voxel::SceneInterface::LineSetPtr &shapeEnd) {
        const math::vector3f emitterDir = _endShapeOffset.lengthSq() > 0.1f ? _endShapeOffset.normalized() : math::vector3f{0, 1, 0};
        const float maxShapeSize = std::max(_startShape.getMaxSize(), _endShape.getMaxSize());
        
        _ptcParams.minXYZ = -1.0f * maxShapeSize * emitterDir.signs();
        _ptcParams.maxXYZ = _endShapeOffset + 1.0f * maxShapeSize * emitterDir.signs();
        
        if (_ptcParams.minXYZ.x > _ptcParams.maxXYZ.x) std::swap(_ptcParams.minXYZ.x, _ptcParams.maxXYZ.x);
        if (_ptcParams.minXYZ.y > _ptcParams.maxXYZ.y) std::swap(_ptcParams.minXYZ.y, _ptcParams.maxXYZ.y);
        if (_ptcParams.minXYZ.z > _ptcParams.maxXYZ.z) std::swap(_ptcParams.minXYZ.z, _ptcParams.maxXYZ.z);
        
        const std::size_t cycleLength = std::size_t(std::ceil(_emissionTimeMs / float(_bakingFrameTimeMs)));
        const std::size_t cyclesInRow = std::size_t(std::ceil((_isLooped ? std::max(_emissionTimeMs, _particleLifeTimeMs) : _emissionTimeMs + _particleLifeTimeMs) / _emissionTimeMs));
        
        _startShape.generate(shapeStart, emitterDir.normalized(), combineRandom(_randomSeed, RND_SHAPE), 1024);
        _endShape.generate(shapeEnd, emitterDir.normalized(), combineRandom(_randomSeed, RND_SHAPE), 1024);
        
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
                        const float ptcEmissionKoeff = float(c) / float(_particlesToEmit);
                        const std::pair<math::vector3f, math::vector3f> shapePoints = _getShapePoints(ptcEmissionKoeff, bornRandom, _endShapeOffset);
                        activeParticles.emplace_back(ActiveParticle {
                            c, bornRandom, currAbsTimeMs, _particleLifeTimeMs, shapePoints.first, shapePoints.second
                        });
                    }
                    
                    bornParticleCount = nextParticleCount;
                }
                
                for (auto ptc = activeParticles.begin(); ptc != activeParticles.end(); ++ptc) {
                    const std::size_t voff = ptc->index * voxel::VERTICAL_PIXELS_PER_PARTICLE;
                    const float ptcAbsTimeMs = currAbsTimeMs - ptc->bornTimeMs;
                    const float lifeKoeff = ptcAbsTimeMs / ptc->lifeTimeMs;
                    
                    if (voff >= mapTextureHeight) {
                        //printf("");
                    }
                    
                    std::uint8_t *m0 = &_mapData[(voff + 0) * mapTextureWidth * 4 + i * 4];
                    std::uint8_t *m1 = &_mapData[(voff + 1) * mapTextureWidth * 4 + i * 4];
                    std::uint8_t *m2 = &_mapData[(voff + 2) * mapTextureWidth * 4 + i * 4];
                    
                    const math::vector3f direction = (ptc->end - ptc->start).normalized();
                    math::vector3f position = ptc->start + direction * _particleSpeed * lifeKoeff;
                    
                    // TODO: ptc position modificators
                    
                    //float tmp;
                    const math::vector3f positionKoeff = 255.0f * (position - _ptcParams.minXYZ) / (_ptcParams.maxXYZ - _ptcParams.minXYZ);
                    
                    auto fract = [](float v) {
                        return v - floor(v);
                    };
                    
                    m0[0] = std::uint8_t(positionKoeff.x);                  // X hi
                    m0[1] = std::uint8_t(255.0f * fract(positionKoeff.x));  // X low
                    m0[2] = std::uint8_t(positionKoeff.y);                  // Y hi
                    m0[3] = std::uint8_t(255.0f * fract(positionKoeff.y));  // Y low
                    m1[0] = std::uint8_t(positionKoeff.z);                  // X hi
                    m1[1] = std::uint8_t(255.0f * fract(positionKoeff.z));  // X low
                    m1[2] = 0.0f;                                           // Angle hi
                    m1[3] = 0.0f;                                           // Angle low
                    
                    m2[0] = 0;                                      // Width
                    m2[1] = 0;                                      // Height
                    m2[3] = loop + 1;                               // Mask: 1 - newborn, 2 - old
                    
                    m2[2] = std::uint8_t(std::min(255.0f, ptcAbsTimeMs / _bakingFrameTimeMs));  // How old the particle is (f.e. 0 - just born, 255 - 255 * BakingTimeSec)
                    
                    if (lifeKoeff < std::numeric_limits<float>::epsilon()) { // mark where the particle starts
                        m2[3] |= 128;
                    }
                }
                
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
        _ptcParams.looped = _isLooped;
        _ptcParams.bakingTimeSec = _bakingFrameTimeMs / 1000.0f;
    }
    
    foundation::RenderTexturePtr Emitter::getMap() const {
        return _mapTexture;
    }
    
    const layouts::ParticlesParams &Emitter::getParams() const {
        return _ptcParams;
    }

    std::pair<math::vector3f, math::vector3f> Emitter::_getShapePoints(float cycleOffset, std::size_t random, const math::vector3f &shapeOffset) const {
        switch (_shapeDistribution) {
            case ShapeDistribution::RANDOM:
            {
                const std::size_t second = getNextRandom(random);
                return std::make_pair(_startShape.points[random % _startShape.points.size()], shapeOffset + _endShape.points[second % _endShape.points.size()]);
            }
            case ShapeDistribution::SHUFFLED:
            {
                const std::size_t rnd = getNextRandom(random);
                const std::size_t startIndex = rnd % _startShape.points.size();
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
    EditorParticlesContext::EditorParticlesContext(API &&api, NodeAccessInterface &nodeAccess, CameraAccessInterface &cameraAccess)
    : _api(std::move(api))
    , _nodeAccess(nodeAccess)
    , _cameraAccess(cameraAccess)
    {
        EditorNode::makeByType[std::size_t(EditorNodeType::PARTICLES)] = [](std::size_t typeIndex) {
            return std::static_pointer_cast<EditorNode>(std::make_shared<EditorNodeParticles>(typeIndex));
        };
        
        _handlers["editor.selectNode"] = &EditorParticlesContext::_selectNode;
        _handlers["editor.static.set_texture_path"] = &EditorParticlesContext::_setTexturePath;
        _handlers["editor.clearNodeSelection"] = &EditorParticlesContext::_clearNodeSelection;
        
        _editorEventsToken = _api.platform->addEditorEventHandler([this](const std::string &msg, const std::string &data) {
            auto handler = _handlers[msg];
            if (handler) {
                _api.platform->logMsg("[Particles Context] msg = '%s' arg = '%s'", msg.data(), data.c_str());
                return (this->*handler)(data);
            }
            return false;
        });
        
        _api.resources->getOrLoadTexture("textures/particles/test", [this](const foundation::RenderTexturePtr &texture) {
            _texture = texture;
        });
    }
    
    EditorParticlesContext::~EditorParticlesContext() {
        _api.platform->removeEventHandler(_editorEventsToken);
    }
    
    void EditorParticlesContext::update(float dtSec) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            currentTime += dtSec;
            node->particles->setTransform(math::transform3f::identity().translated(node->position));
            node->particles->setTime(currentTime, 0.0f);
            _movingTool->setBase(node->position);
            _shapeStartLineset->setPosition(node->position);
            _shapeEndLineset->setPosition(node->position + _movingTool->getPosition());
            _shapeConnectLineset->setPosition(node->position);
            _shapeConnectLineset->setLine(0, {0, 0, 0}, _movingTool->getPosition(), {0.4f, 0.4f, 0.0f, 0.1f}, false);
            _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
        }
    }

    void EditorParticlesContext::_endShapeDragFinished() {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            node->emitter.setEndShapeOffset(_movingTool->getPosition());
            node->emitter.refresh(_api.rendering, _shapeStartLineset, _shapeEndLineset);
            node->particles = _api.scene->addParticles(_texture, node->emitter.getMap(), node->emitter.getParams());
            node->particles->setTime(10.0f, 0.0f);
            _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
        }
    }

    bool EditorParticlesContext::_selectNode(const std::string &data) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            _movingTool = std::make_unique<MovingTool>(_api, _cameraAccess, node->endShapeOffset, 0.4f);
            _movingTool->setEditorMsg("engine.endShapeOffset");
            _movingTool->setOnDragEnd([this] {
                this->_endShapeDragFinished();
            });
            _shapeConnectLineset = _api.scene->addLineSet();
            _shapeStartLineset = _api.scene->addLineSet();
            _shapeEndLineset = _api.scene->addLineSet();
            _api.platform->sendEditorMsg("engine.nodeSelected", data + " inspect_particles " + node->texturePath);

            node->emitter.setEndShapeOffset(_movingTool->getPosition());
            node->emitter.refresh(_api.rendering, _shapeStartLineset, _shapeEndLineset);
            node->particles = _api.scene->addParticles(_texture, node->emitter.getMap(), node->emitter.getParams());
            node->particles->setTime(10.0f, 0.0f);
        }
        else {
            _movingTool = nullptr;
            _shapeConnectLineset = nullptr;
            _shapeStartLineset = nullptr;
            _shapeEndLineset = nullptr;
        }
        return false;
    }
    
    bool EditorParticlesContext::_setTexturePath(const std::string &data) {
        return true;
    }
    
    bool EditorParticlesContext::_clearNodeSelection(const std::string &data) {
        _movingTool = nullptr;
        _shapeConnectLineset = nullptr;
        _shapeStartLineset = nullptr;
        _shapeEndLineset = nullptr;
        return false;
    }
}


