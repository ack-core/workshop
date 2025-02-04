
#include "editor_particles_context.h"
#include <list>

/*
namespace game {
    namespace {
        const std::uint32_t PIXELS_PER_PARTICLE = 2;
    }
    
    Emitter::Emitter() {
    
    }
    Emitter::~Emitter() {
    
    }
    
    void Emitter::setEndShapeOffset(const math::vector3f &offset) {
        _endShapeOffset = offset;
        _bbMin.x = std::min(0.0f, _endShapeOffset.x);
        _bbMin.y = std::min(0.0f, _endShapeOffset.y);
        _bbMin.z = std::min(0.0f, _endShapeOffset.z);
        _bbMax.x = std::max(0.0f, _endShapeOffset.x);
        _bbMax.y = std::max(0.0f, _endShapeOffset.y);
        _bbMax.z = std::max(0.0f, _endShapeOffset.z);
    }
    void Emitter::refresh() {
        const std::size_t mapTextureWidth = std::size_t(std::ceil((_emissionTimeMs + _particleLifeTimeMs) / _emissionTimeMs)) * std::uint32_t(std::ceil(_emissionTimeMs / float(_bakingFrameTimeMs)));
        const std::size_t mapTextureHeight = PIXELS_PER_PARTICLE * _particlesToEmit;
        
        _mapData.resize(mapTextureWidth * mapTextureHeight * 4);
        std::fill(_mapData.begin(), _mapData.end(), 0);
        
        std::list<ActiveParticle> activeParticles;
        std::size_t bornParticleCount = 0;
        
        for (std::size_t i = 0; i < mapTextureWidth; i++) {
            const float currAbsTimeMs = float((i + 0) * _bakingFrameTimeMs);
            const float nextAbsTimeMs = float((i + 1) * _bakingFrameTimeMs);
            const std::size_t nextParticleCount = std::size_t(float(_particlesToEmit) * _getGraphFilling(_emissionGraphType, nextAbsTimeMs / _emissionTimeMs));

            for (std::size_t i = bornParticleCount; i < nextParticleCount; i++) {
                std::pair<math::vector3f, math::vector3f> shapePoints = _getShapePoints(i);
                activeParticles.emplace_back(ActiveParticle {
                    i, 0, currAbsTimeMs, _particleLifeTimeMs, shapePoints.first, shapePoints.second
                });
            }

            for (auto ptc = activeParticles.begin(); ptc != activeParticles.end(); ) {
                const std::size_t voff = ptc->index * PIXELS_PER_PARTICLE;
                const float lifeKoeff = (currAbsTimeMs - ptc->bornTimeMs) / ptc->lifeTimeMs;
                
                std::uint8_t *m0 = &_mapData[(voff + 0) * mapTextureWidth + i];
                std::uint8_t *m1 = &_mapData[(voff + 1) * mapTextureWidth + i];
                
                if (lifeKoeff > 1.0f) {
                    ptc = activeParticles.erase(ptc);
                }
                else {
                    math::vector3f position = ptc->start + (ptc->end - ptc->start) * lifeKoeff;
                    
                    // todo: ptc position modificators
                    
                    const math::vector3f positionKoeff = (position - _bbMin) / (_bbMax - _bbMin);
                    
                    m0[0] = std::uint8_t(positionKoeff.x * 255.0f);
                    m0[1] = std::uint8_t(positionKoeff.y * 255.0f);
                    m0[2] = std::uint8_t(positionKoeff.z * 255.0f);
                    m0[3] = 0;
                    m1[0] = 0;
                    m1[1] = 0;
                    m1[2] = 0;
                    m1[3] = 0xff;
                    
                    ++ptc;
                }
            }
            
            bornParticleCount = nextParticleCount;
        }
    }

    float Emitter::_getGraphFilling(GraphType type, float t) const {
        return std::min(std::max(t, 0.0f), 1.0f);
    }
    
    std::pair<math::vector3f, math::vector3f> Emitter::_getShapePoints(std::size_t ptcIndex) const {
        return std::make_pair(math::vector3f{0, 0, 0}, _endShapeOffset);
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
                _api.platform->logMsg("[Static Mesh Context] msg = '%s' arg = '%s'", msg.data(), data.c_str());
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
            _movingTool->setBase(node->position);
            _lineset->setPosition(node->position);
            _lineset->setLine(0, {0, 0, 0}, _movingTool->getPosition(), {0.3f, 0.3f, 0.3f, 0.3f}, false);
        }
    }

    void EditorParticlesContext::_endShapeDragFinished() {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            node->emitter.setEndShapeOffset(_movingTool->getPosition());
            node->emitter.refresh();
        }
    }

    bool EditorParticlesContext::_selectNode(const std::string &data) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            _movingTool = std::make_unique<MovingTool>(_api, _cameraAccess, node->endShapeOffset, 0.4f);
            _movingTool->setEditorMsg("engine.endShapeOffset");
            _movingTool->setOnDragEnd([this] {
                this->_endShapeDragFinished();
            });
            _lineset = _api.scene->addLineSet();
            _api.platform->sendEditorMsg("engine.nodeSelected", data + " inspect_particles " + node->texturePath);
            return true;
        }
        else {
            _movingTool = nullptr;
            _lineset = nullptr;
        }
        return false;
    }
    
    bool EditorParticlesContext::_setTexturePath(const std::string &data) {
        return true;
    }
    
    bool EditorParticlesContext::_clearNodeSelection(const std::string &data) {
        _movingTool = nullptr;
        _lineset = nullptr;
        return false;
    }
}

*/
