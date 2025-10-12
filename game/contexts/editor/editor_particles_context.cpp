
#include "editor_particles_context.h"
#include "utils.h"
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
                float r = 0.5f * args.x;
                
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
                const math::vector3f p0 = math::vector3f(0.5f * args.x * std::cosf(koeff0), 0.0f, 0.5f * args.x * std::sinf(koeff0)).transformed(rotation);
                const math::vector3f p1 = math::vector3f(0.5f * args.x * std::cosf(koeff1), 0.0f, 0.5f * args.x * std::sinf(koeff1)).transformed(rotation);
                lineSet->setLine(i, p0, p1, {0.4f, 0.4f, 0.0f, 0.1f});
            }
            lineSet->capLineCount(36);
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
                        
                        if (dimCount) {
                            random = getNextRandom(random);
                            koeff.flat[dimIndexes[i % dimCount]] = random % 2000 >= 1000 ? 0.5f : -0.5f;
                        }
                        
                        points.emplace_back(args * koeff);
                    }
                }

            }
            
            // lineset
            const math::vector3f bbmin = -0.5f * args;
            const math::vector3f bbmax = 0.5f * args;
            
            lineSet->setLine(0,  math::vector3f(bbmin.x, bbmin.y, bbmin.z), math::vector3f(bbmax.x, bbmin.y, bbmin.z), {0.4f, 0.4f, 0.0f, 0.07f});
            lineSet->setLine(1,  math::vector3f(bbmin.x, bbmin.y, bbmin.z), math::vector3f(bbmin.x, bbmin.y, bbmax.z), {0.4f, 0.4f, 0.0f, 0.07f});
            lineSet->setLine(2,  math::vector3f(bbmax.x, bbmin.y, bbmax.z), math::vector3f(bbmax.x, bbmin.y, bbmin.z), {0.4f, 0.4f, 0.0f, 0.07f});
            lineSet->setLine(3,  math::vector3f(bbmax.x, bbmin.y, bbmax.z), math::vector3f(bbmin.x, bbmin.y, bbmax.z), {0.4f, 0.4f, 0.0f, 0.07f});
            lineSet->setLine(4,  math::vector3f(bbmin.x, bbmax.y, bbmin.z), math::vector3f(bbmax.x, bbmax.y, bbmin.z), {0.4f, 0.4f, 0.0f, 0.07f});
            lineSet->setLine(5,  math::vector3f(bbmin.x, bbmax.y, bbmin.z), math::vector3f(bbmin.x, bbmax.y, bbmax.z), {0.4f, 0.4f, 0.0f, 0.07f});
            lineSet->setLine(6,  math::vector3f(bbmax.x, bbmax.y, bbmax.z), math::vector3f(bbmax.x, bbmax.y, bbmin.z), {0.4f, 0.4f, 0.0f, 0.07f});
            lineSet->setLine(7,  math::vector3f(bbmax.x, bbmax.y, bbmax.z), math::vector3f(bbmin.x, bbmax.y, bbmax.z), {0.4f, 0.4f, 0.0f, 0.07f});
            lineSet->setLine(8,  math::vector3f(bbmin.x, bbmin.y, bbmin.z), math::vector3f(bbmin.x, bbmax.y, bbmin.z), {0.4f, 0.4f, 0.0f, 0.07f});
            lineSet->setLine(9,  math::vector3f(bbmax.x, bbmin.y, bbmin.z), math::vector3f(bbmax.x, bbmax.y, bbmin.z), {0.4f, 0.4f, 0.0f, 0.07f});
            lineSet->setLine(10, math::vector3f(bbmax.x, bbmin.y, bbmax.z), math::vector3f(bbmax.x, bbmax.y, bbmax.z), {0.4f, 0.4f, 0.0f, 0.07f});
            lineSet->setLine(11, math::vector3f(bbmin.x, bbmin.y, bbmax.z), math::vector3f(bbmin.x, bbmax.y, bbmax.z), {0.4f, 0.4f, 0.0f, 0.07f});

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
    
    void Emitter::setParameters(const resource::EmitterDescription &desc) {
        _emissionTimeMs = desc.emissionTimeMs;
        _particleLifeTimeMs = desc.particleLifeTimeMs;
        _bakingFrameTimeMs = desc.bakingFrameTimeMs;
        _particlesToEmit = desc.particlesToEmit;
        _isLooped = desc.looped;
        _randomSeed = desc.randomSeed;
        _startShape.type = Shape::Type(desc.startShapeType);
        _startShape.fill = desc.startShapeFill;
        _startShape.args = desc.startShapeArgs;
        _endShape.type = Shape::Type(desc.endShapeType);
        _endShape.fill = desc.endShapeFill;
        _endShape.args = desc.endShapeArgs;
        _endShapeOffset = desc.endShapeOffset;
        _shapeDistribution = ShapeDistribution(desc.shapeDistributionType);
        _ptcParams.orientation = voxel::ParticlesParams::ParticlesOrientation(desc.particleOrientation);
        _ptcParams.additiveBlend = desc.additiveBlend;
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
                    m1[2] = 0;                                              // Alpha
                    m1[3] = 0;                                              //
                    
                    m2[0] = 255;                                      // Width
                    m2[1] = 255;                                      // Height
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
        _handlers["editor.clearNodeSelection"] = &EditorParticlesContext::_clearNodeSelection;
        _handlers["editor.setPath"] = &EditorParticlesContext::_setResourcePath;
        _handlers["editor.startEditing"] = &EditorParticlesContext::_startEditing;
        _handlers["editor.stopEditing"] = &EditorParticlesContext::_stopEditing;
        _handlers["editor.reload"] = &EditorParticlesContext::_reload;
        _handlers["editor.particles.emission"] = &EditorParticlesContext::_emissionSet;
        _handlers["editor.particles.visual"] = &EditorParticlesContext::_visualSet;
        _handlers["editor.resource.save"] = &EditorParticlesContext::_save;

        _editorEventsToken = _api.platform->addEditorEventHandler([this](const std::string &msg, const std::string &data) {
            auto handler = _handlers[msg];
            if (handler) {
                _api.platform->logMsg("[Particles Context] msg = '%s' arg = '%s'", msg.data(), data.c_str());
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
                if (_endShapeTool && node->currentDesc.has_value()) {
                    _endShapeTool->setBase(node->position);
                    _shapeStartLineset->setPosition(node->position);
                    _shapeEndLineset->setPosition(node->position + _endShapeTool->getPosition());
                    _shapeConnectLineset->setPosition(node->position);
                    _shapeConnectLineset->setLine(0, {0, 0, 0}, _endShapeTool->getPosition(), {0.4f, 0.4f, 0.0f, 0.1f}, false);
                    currentTime += dtSec;
                    node->particles->setTime(currentTime, 0.0f);
                }
                
                node->particles->setTransform(math::transform3f::identity().translated(node->position));
                _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
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
            node->currentDesc->endShapeOffset = off;
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
            voxel::ParticlesParams parameters;
            resource::EmitterDescription &desc = *node.originDesc;
            parameters.additiveBlend = desc.additiveBlend;
            parameters.orientation = voxel::ParticlesParams::ParticlesOrientation(desc.particleOrientation);
            parameters.bakingTimeSec = float(desc.bakingFrameTimeMs) / 1000.0f;
            parameters.minXYZ = desc.minXYZ;
            parameters.maxXYZ = desc.maxXYZ;
            parameters.maxSize = desc.maxSize;
            node.particles = _api.scene->addParticles(node.texture, node.map, parameters);
            node.particles->setTime((desc.emissionTimeMs / 1000.0f) * 1.9f, 0.0f); // set almost at the end of second cycle
        }
    }

    bool EditorParticlesContext::_selectNode(const std::string &data) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            _api.platform->sendEditorMsg("engine.nodeSelected", data + " inspect_particles " + node->emitterPath);
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
            node->emitterPath = data;
            
            _api.resources->getOrLoadEmitter(data.c_str(), [node, this](const resource::EmitterDescriptionPtr &desc, const foundation::RenderTexturePtr &m, const foundation::RenderTexturePtr &t) {
                if (desc) {
                    node->originDesc = *desc;
                    node->texture = t;
                    node->map = m;
                    
                    if (node->map) {
                        _recreateParticles(*node, false);
                        _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
                    }
                }
            });
            return true;
        }
        return false;
    }
    
    bool EditorParticlesContext::_startEditing(const std::string &data) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            if (node->originDesc.has_value()) {
                node->currentDesc = node->originDesc;
                
                _endShapeTool = std::make_unique<MovingTool>(_api, _cameraAccess, node->endShapeOffset, 0.4f);
                _endShapeTool->setEditorMsg("engine.endShapeOffset");
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
                
                std::string args;
                args += std::to_string(node->currentDesc->emissionTimeMs) + " ";
                args += std::to_string(node->currentDesc->particleLifeTimeMs) + " ";
                args += std::to_string(node->currentDesc->bakingFrameTimeMs) + " ";
                args += std::to_string(node->currentDesc->particlesToEmit) + " ";
                args += std::to_string(int(node->currentDesc->looped)) + " ";
                args += std::to_string(node->currentDesc->randomSeed) + " ";

                args += std::to_string(int(node->currentDesc->startShapeType)) + " ";
                args += std::to_string(int(node->currentDesc->startShapeFill)) + " ";
                args += util::strstream::ftos(node->currentDesc->startShapeArgs.x) + " ";
                args += util::strstream::ftos(node->currentDesc->startShapeArgs.y) + " ";
                args += util::strstream::ftos(node->currentDesc->startShapeArgs.z) + " ";

                args += std::to_string(int(node->currentDesc->endShapeType)) + " ";
                args += std::to_string(int(node->currentDesc->endShapeFill)) + " ";
                args += util::strstream::ftos(node->currentDesc->endShapeArgs.x) + " ";
                args += util::strstream::ftos(node->currentDesc->endShapeArgs.y) + " ";
                args += util::strstream::ftos(node->currentDesc->endShapeArgs.z) + " ";
                args += util::strstream::ftos(node->currentDesc->endShapeOffset.x) + " ";
                args += util::strstream::ftos(node->currentDesc->endShapeOffset.y) + " ";
                args += util::strstream::ftos(node->currentDesc->endShapeOffset.z) + " ";

                args += std::to_string(int(node->currentDesc->shapeDistributionType)) + " ";
                args += std::to_string(int(node->currentDesc->particleOrientation)) + " ";

                args += node->currentDesc->texturePath;

                _api.platform->sendEditorMsg("engine.particles.editing", args);
                _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
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
                node->endShapeOffset = node->originDesc->endShapeOffset;
            }
        }
        _clearEditingTools();
        return false;
    }
    
    bool EditorParticlesContext::_reload(const std::string &data) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            _api.resources->removeEmitter(node->emitterPath.data());
            
            // updating all emitter nodes
            _nodeAccess.forEachNode([this](const std::shared_ptr<EditorNode> &node) {
                std::shared_ptr<EditorNodeParticles> emitterNode = std::dynamic_pointer_cast<EditorNodeParticles>(node);
                if (emitterNode && emitterNode->particles) {
                    const char *path = emitterNode->emitterPath.c_str();
                    _api.resources->getOrLoadEmitter(path, [emitterNode, this](const resource::EmitterDescriptionPtr &desc, const foundation::RenderTexturePtr &m, const foundation::RenderTexturePtr &t) {
                        if (desc) {
                            emitterNode->originDesc = *desc;
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
        }
        return false;
    }
    
    bool EditorParticlesContext::_save(const std::string &data) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            const math::vector3f &sargs = node->currentDesc->startShapeArgs;
            const math::vector3f &eargs = node->currentDesc->endShapeArgs;
            const math::vector3f &eoff = node->currentDesc->endShapeOffset;
            const math::vector3f &bmin = node->currentDesc->minXYZ;
            const math::vector3f &bmax = node->currentDesc->maxXYZ;
            const math::vector2f &sz = node->currentDesc->maxSize;

            _savingCfg = "emitter {\r\n";
            _savingCfg += std::string("    looped : bool = ") + (node->currentDesc->looped ? "true" : "false") + "\r\n";
            _savingCfg += std::string("    emissionTimeMs : integer = ") + std::to_string(node->currentDesc->emissionTimeMs) + "\r\n";
            _savingCfg += std::string("    particleLifeTimeMs : integer = ") + std::to_string(node->currentDesc->particleLifeTimeMs) + "\r\n";
            _savingCfg += std::string("    bakingFrameTimeMs : integer = ") + std::to_string(node->currentDesc->bakingFrameTimeMs) + "\r\n";
            _savingCfg += std::string("    particlesToEmit : integer = ") + std::to_string(node->currentDesc->particlesToEmit) + "\r\n";
            _savingCfg += std::string("    randomSeed : integer = ") + std::to_string(node->currentDesc->randomSeed) + "\r\n";
            
            _savingCfg += std::string("    particleStartSpeed : number = 10.0\r\n"); // TODO: speed!
            
            _savingCfg += std::string("    texture : string = \"") + node->currentDesc->texturePath + "\"\r\n";
            _savingCfg += std::string("    startShapeType : integer = ") + std::to_string(node->currentDesc->startShapeType) + "\r\n";
            _savingCfg += std::string("    startShapeFill : bool = ") + std::to_string(int(node->currentDesc->startShapeFill)) + "\r\n";
            _savingCfg += std::string("    startShapeArgs : vector3f = ");
            _savingCfg += util::strstream::ftos(sargs.x) + " " + util::strstream::ftos(sargs.y) + " " + util::strstream::ftos(sargs.z) + "\r\n";
            _savingCfg += std::string("    endShapeType : integer = ") + std::to_string(node->currentDesc->endShapeType) + "\r\n";
            _savingCfg += std::string("    endShapeFill : bool = ") + std::to_string(int(node->currentDesc->endShapeFill)) + "\r\n";
            _savingCfg += std::string("    endShapeArgs : vector3f = ");
            _savingCfg += util::strstream::ftos(eargs.x) + " " + util::strstream::ftos(eargs.y) + " " + util::strstream::ftos(eargs.z) + "\r\n";
            _savingCfg += std::string("    endShapeOffset : vector3f = ");
            _savingCfg += util::strstream::ftos(eoff.x) + " " + util::strstream::ftos(eoff.y) + " " + util::strstream::ftos(eoff.z) + "\r\n";
            _savingCfg += std::string("    shapeDistributionType : integer = ") + std::to_string(node->currentDesc->shapeDistributionType) + "\r\n";
            _savingCfg += std::string("    particleOrientation : integer = ") + std::to_string(node->currentDesc->particleOrientation) + "\r\n";
            _savingCfg += std::string("    additiveBlend : bool = ") + (node->currentDesc->additiveBlend ? "true" : "false") + "\r\n";
            _savingCfg += std::string("    minXYZ : vector3f = ");
            _savingCfg += util::strstream::ftos(bmin.x) + " " + util::strstream::ftos(bmin.y) + " " + util::strstream::ftos(bmin.z) + "\r\n";
            _savingCfg += std::string("    maxXYZ : vector3f = ");
            _savingCfg += util::strstream::ftos(bmax.x) + " " + util::strstream::ftos(bmax.y) + " " + util::strstream::ftos(bmax.z) + "\r\n";
            _savingCfg += std::string("    maxSize : vector2f = ");
            _savingCfg += util::strstream::ftos(sz.x) + " " + util::strstream::ftos(sz.y) + "\r\n";
            _savingCfg += "}\r\n";
            
            _api.platform->saveFile((node->emitterPath + ".txt").c_str(), reinterpret_cast<const std::uint8_t *>(_savingCfg.data()), _savingCfg.length(), [this, node](bool result) {
                const std::size_t width = node->emitter.getMap()->getWidth();
                const std::size_t height = node->emitter.getMap()->getHeight();
                auto tgaResult = editor::writeTGA(node->emitter.getMapRaw(), width, height);
                _savingMap = std::move(tgaResult.first);
            
                _api.platform->saveFile((node->emitterPath + ".tga").c_str(), _savingMap.get(), tgaResult.second, [this, node](bool result) {
                    _api.platform->sendEditorMsg("engine.saved", std::to_string(int(result)));
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
            if (input >> node->currentDesc->emissionTimeMs >> node->currentDesc->particleLifeTimeMs >> node->currentDesc->bakingFrameTimeMs) {
                if (input >> node->currentDesc->particlesToEmit >> node->currentDesc->looped) {
                    node->emitter.setParameters(*node->currentDesc);
                    node->emitter.refresh(_api.rendering, _shapeStartLineset, _shapeEndLineset);
                    _recreateParticles(*node, true);
                }
            }
            return true;
        }
        return false;
    }
    bool EditorParticlesContext::_visualSet(const std::string &data) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            util::strstream input(data.c_str(), data.length());
            bool success = false;
            
            success |= input >> node->currentDesc->startShapeType >> node->currentDesc->startShapeFill;
            success |= input >> node->currentDesc->startShapeArgs.x >> node->currentDesc->startShapeArgs.y >> node->currentDesc->startShapeArgs.z;
            success |= input >> node->currentDesc->endShapeType >> node->currentDesc->endShapeFill;
            success |= input >> node->currentDesc->endShapeArgs.x >> node->currentDesc->endShapeArgs.y >> node->currentDesc->endShapeArgs.z;
            success |= input >> node->currentDesc->endShapeOffset.x >> node->currentDesc->endShapeOffset.y >> node->currentDesc->endShapeOffset.z;
            success |= input >> node->currentDesc->shapeDistributionType >> node->currentDesc->particleOrientation;

            if (success) {
                node->endShapeOffset = node->currentDesc->endShapeOffset;
                node->emitter.setParameters(*node->currentDesc);
                node->emitter.refresh(_api.rendering, _shapeStartLineset, _shapeEndLineset);
                _recreateParticles(*node, true);
            }
            return true;
        }
        return false;
    }
}


