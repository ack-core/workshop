
#include "editor_particles_context.h"

namespace game {
    namespace {
        auto getEmitterDir(const math::vector3f &endShapeOffset) -> math::vector3f {
            return endShapeOffset.lengthSq() > 0.1f ? endShapeOffset.normalized() : math::vector3f{0, 1, 0};
        }
        auto getDirRotation(const math::vector3f &dir) -> math::transform3f {
            const float a = math::vector3f(0, 1, 0).angleTo(dir);
            const math::vector3f axis = math::vector3f(0, 1, 0).cross(dir.normalized()).normalized();
            return math::transform3f(axis, -a);
        }
        void fillShapeLineset(const core::SceneInterface::LineSetPtr &lineSet, core::EmitterBaker::ShapeType type, const math::vector3f &args, const math::vector3f &dir) {
            if (type == core::EmitterBaker::ShapeType::DISK) {
                lineSet->fillAsCircle(36, 0.5f * args.x, {0.7f, 0.7f, 0.0f, 0.6f});
                lineSet->setTransform(getDirRotation(dir));
            }
            else if (type == core::EmitterBaker::ShapeType::BOX) {
                int dimCount = 0;
                for (int i = 0; i < 3; i++) {
                    dimCount += args.flat[i] > std::numeric_limits<float>::epsilon() ? 1 : 0;
                }

                const math::vector3f bbmin = -0.5f * args;
                const math::vector3f bbmax = 0.5f * args;
                const float alpha = 0.3f + 0.1f * float(std::max(dimCount, 1));
                lineSet->clear();
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
            }
        }
    }

    void EditorNodeParticles::update(float dtSec) {
        globalPosition = localPosition + (parent ? parent->globalPosition : math::vector3f(0, 0, 0));
        if (particles) {
            particles->setTransform(math::transform3f::identity().translated(globalPosition));
            particles->setTimings(currentTime, 0.0f, {});
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
                        particles = api.scene->addParticles(texture, map, *originDesc);
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
        // WARNING: editor can only be single-threaded
        _rendering = foundation::RenderingInterface::instance(foundation::PlatformInterface::instance());

        EditorNode::makeByType[std::size_t(core::WorldInterface::NodeType::PARTICLES)] = [](std::size_t typeIndex) {
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
                _api.platform->logMsg("[PARTICLES] msg = '%s' arg = '%s'", msg.data(), data.c_str());
                if ((this->*handler)(data)) {
                    return true;
                }
                _api.platform->logMsg("[PARTICLES] msg skipped");
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
                    _shapeConnectLineset->setLine(0, {0, 0, 0}, _endShapeTool->getPosition(), {0.7f, 0.7f, 0.0f, 0.6f});
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
            node->emitter->setEndShapeOffset(off);
            node->emitter->refresh();
            _refreshShapeLinesets(*node->emitter);
            _recreateParticles(*node, true);
            _api.platform->sendEditorMsg("engine.ptcEndOffset", util::strstream::ftos(off.x) + " " + util::strstream::ftos(off.y) + " " + util::strstream::ftos(off.z));
        }
    }

    void EditorParticlesContext::_refreshShapeLinesets(const core::EmitterBaker &emitter) {
        const util::Description &desc = emitter.getDescription();
        const math::vector3f dir = getEmitterDir(desc.getVector3f("endShapeOffset", {}));
        fillShapeLineset(_shapeStartLineset, static_cast<core::EmitterBaker::ShapeType>(desc.getInteger("startShapeType", 0)), desc.getVector3f("startShapeArgs", {}), dir);
        fillShapeLineset(_shapeEndLineset, static_cast<core::EmitterBaker::ShapeType>(desc.getInteger("endShapeType", 0)), desc.getVector3f("endShapeArgs", {}), dir);
    }

    void EditorParticlesContext::_recreateParticles(EditorNodeParticles &node, bool editing) {
        if (editing) {
            node.particles = _api.scene->addParticles(node.texture, node.emitter->getMap(_rendering), node.emitter->getDescription());
        }
        else {
            node.particles = _api.scene->addParticles(node.texture, node.map, *node.originDesc);
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

                node->emitter = core::makeEmitterBaker();
                node->emitter->setParameters(node->currentDesc.value());
                node->emitter->setEndShapeOffset(_endShapeTool->getPosition());
                node->emitter->refresh();
                _refreshShapeLinesets(*node->emitter);
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
                auto tgaResult = util::writeTGA(node->emitter->getMapRaw(), node->emitter->getMapWidth(), node->emitter->getMapHeight());
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
                node->emitter->setParameters(*node->currentDesc);
                node->emitter->refresh();
                desc.setVector3f("minXYZ", node->emitter->getMinXYZ());
                desc.setVector3f("maxXYZ", node->emitter->getMaxXYZ());
                desc.setVector2f("maxSize", node->emitter->getMaxSize());
                _refreshShapeLinesets(*node->emitter);
                _recreateParticles(*node, true);
            }
            return true;
        }
        return false;
    }
    bool EditorParticlesContext::_visualSet(const std::string &data) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            util::strstream input(data.c_str(), data.length());
            std::string ptcTexture;
            math::vector3f startShapeArgs, endShapeArgs, endShapeOffset;
            int startShapeType, endShapeType, shapeDistributionType, particleOrientation;
            bool startShapeFill, endShapeFill;
            bool success = false;

            success |= input >> startShapeType >> startShapeFill;
            success |= input >> startShapeArgs.x >> startShapeArgs.y >> startShapeArgs.z;
            success |= input >> endShapeType >> endShapeFill;
            success |= input >> endShapeArgs.x >> endShapeArgs.y >> endShapeArgs.z;
            success |= input >> endShapeOffset.x >> endShapeOffset.y >> endShapeOffset.z;
            success |= input >> shapeDistributionType >> particleOrientation >> ptcTexture;
            
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
                desc.setString("texture", ptcTexture);
                node->endShapeOffset = endShapeOffset;
                node->emitter->setParameters(*node->currentDesc);
                node->emitter->refresh();
                desc.setVector3f("minXYZ", node->emitter->getMinXYZ());
                desc.setVector3f("maxXYZ", node->emitter->getMaxXYZ());
                desc.setVector2f("maxSize", node->emitter->getMaxSize());
                _refreshShapeLinesets(*node->emitter);
                _api.resources->getOrLoadTexture(ptcTexture.c_str(), [this, node](const foundation::RenderTexturePtr &t) {
                    node->texture = t;
                    _recreateParticles(*node, true);
                });
            }
            return true;
        }
        return false;
    }
    bool EditorParticlesContext::_graphSet(const std::string &data, const std::string &name) {
        if (std::shared_ptr<EditorNodeParticles> node = std::dynamic_pointer_cast<EditorNodeParticles>(_nodeAccess.getSelectedNode().lock())) {
            util::Description &desc = node->currentDesc.value();
            desc.setString(name.c_str(), data);
            node->emitter->setParameters(*node->currentDesc);
            node->emitter->refresh();
            desc.setVector3f("minXYZ", node->emitter->getMinXYZ());
            desc.setVector3f("maxXYZ", node->emitter->getMaxXYZ());
            desc.setVector2f("maxSize", node->emitter->getMaxSize());
            _refreshShapeLinesets(*node->emitter);
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


