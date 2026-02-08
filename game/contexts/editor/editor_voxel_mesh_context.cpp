
#include "editor_voxel_mesh_context.h"
#include "utils.h"

namespace game {
    void EditorNodeVoxelMesh::update(float dtSec) {
        if (mesh) {
            globalPosition = localPosition + (parent ? parent->globalPosition : math::vector3f(0, 0, 0));
            mesh->setPosition(globalPosition);
        }
    }
    void EditorNodeVoxelMesh::setResourcePath(const API &api, const std::string &path) {
        if (path.empty()) {
            resourcePath = EDITOR_EMPTY_RESOURCE_PATH;
            mesh = nullptr;
        }
        else {
            resourcePath = path;
            api.resources->getOrLoadVoxelMesh(path.c_str(), [this, &api](const std::vector<foundation::RenderDataPtr> &data, const util::Description& desc) {
                mesh = api.scene->addVoxelMesh(data, desc);
                api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
            });
        }
    }

    EditorVoxelMeshContext::EditorVoxelMeshContext(API &&api, NodeAccessInterface &nodeAccess) : _api(std::move(api)), _nodeAccess(nodeAccess) {
        EditorNode::makeByType[std::size_t(voxel::WorldInterface::NodeType::VOXEL)] = [](std::size_t typeIndex) {
            return std::static_pointer_cast<EditorNode>(std::make_shared<EditorNodeVoxelMesh>(typeIndex));
        };
        
        _handlers["editor.selectNode"] = &EditorVoxelMeshContext::_selectNode;
        _handlers["editor.setPath"] = &EditorVoxelMeshContext::_setResourcePath;
        _handlers["editor.startEditing"] = &EditorVoxelMeshContext::_startEditing;
        _handlers["editor.stopEditing"] = &EditorVoxelMeshContext::_stopEditing;
        _handlers["editor.reloadResources"] = &EditorVoxelMeshContext::_reload;
        _handlers["editor.mesh.offset"] = &EditorVoxelMeshContext::_meshOffset;
        _handlers["editor.resource.save"] = &EditorVoxelMeshContext::_save;
        _handlers["editor.mesh.animationRemoved"] = &EditorVoxelMeshContext::_animationRemoved;
        _handlers["editor.mesh.animationSelected"] = &EditorVoxelMeshContext::_animationSelected;
        _handlers["editor.mesh.animationParameters"] = &EditorVoxelMeshContext::_animationParameters;
        _handlers["editor.mesh.animationPlay"] = &EditorVoxelMeshContext::_animationPlay;
        
        _editorEventsToken = _api.platform->addEditorEventHandler([this](const std::string &msg, const std::string &data) {
            auto handler = _handlers[msg];
            if (handler) {
                return (this->*handler)(data);
            }
            return false;
        });
    }
    
    EditorVoxelMeshContext::~EditorVoxelMeshContext() {
        _api.platform->removeEventHandler(_editorEventsToken);
    }
    
    void EditorVoxelMeshContext::update(float dtSec) {
        if (std::shared_ptr<EditorNodeVoxelMesh> node = std::dynamic_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock())) {
            if (node->mesh && _currentAnimationTime.has_value()) {
                auto anim = node->animations.find(_currentAnimation);
                if (anim != node->animations.end()) {
                    const float length = float(anim->second.y - anim->second.x + 1);
                    float frameOffset = std::max(0.0f, length * (_currentAnimationTime.value() * 1000.0f / float(anim->second.z)));
                    if (frameOffset > length) {
                        frameOffset = 0.0f;
                        if (_animationLooped) {
                            _currentAnimationTime.value() -= float(anim->second.z) / 1000.0f;
                        }
                    }
                    node->mesh->setFrame(anim->second.x + std::uint32_t(frameOffset));
                    _currentAnimationTime.value() += dtSec;
                }
            }
        }
    }
    
    bool EditorVoxelMeshContext::_selectNode(const std::string &data) {
        if (std::shared_ptr<EditorNodeVoxelMesh> node = std::dynamic_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock())) {
            _api.platform->sendEditorMsg("engine.nodeSelected", data + " inspect_mesh " + node->resourcePath);
        }
        return false;
    }
    
    bool EditorVoxelMeshContext::_setResourcePath(const std::string &data) {
        if (std::shared_ptr<EditorNodeVoxelMesh> node = std::dynamic_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock())) {
            node->setResourcePath(_api, data);
            return true;
        }
        return false;
    }
        
    bool EditorVoxelMeshContext::_startEditing(const std::string &data) {
        std::shared_ptr<EditorNodeVoxelMesh> node = std::dynamic_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock());
        if (node && node->mesh) {
            const math::vector3f offset = node->mesh->getCenterOffset();
            const util::Description *anims =  node->mesh->getDescription().getDescription("animations");

            _api.platform->sendEditorMsg("engine.mesh.editing", util::strstream::ftos(offset.x) + " " + util::strstream::ftos(offset.y) + " " + util::strstream::ftos(offset.z));
            if (anims) {
                node->animations = anims->getVector3is();
                for (auto &anim : node->animations) {
                    const math::vector3i &args = anim.second;
                    _api.platform->sendEditorMsg("engine.mesh.animationUpdate", anim.first + " " + std::to_string(args.x) + " " + std::to_string(args.y) + " " + std::to_string(args.z) + " new");
                    _currentAnimation = anim.first;
                }
                _currentAnimationTime.reset();
            }
            
            return true;
        }
        
        return false;
    }
    
    bool EditorVoxelMeshContext::_stopEditing(const std::string &data) {
        std::shared_ptr<EditorNodeVoxelMesh> node = std::dynamic_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock());
        if (node && node->mesh) {
            node->mesh->resetOffset();
            node->animations.clear();
            _currentAnimation = "";
            _currentAnimationTime.reset();
            return true;
        }
        return false;
    }
        
    bool EditorVoxelMeshContext::_reload(const std::string &data) {
        std::shared_ptr<EditorNodeVoxelMesh> node = std::dynamic_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock());
        if (node && node->mesh) {
            _api.resources->removeMesh(node->resourcePath.data());
            _api.resources->removeMesh(data.c_str());
            
            // updating all mesh nodes
            _nodeAccess.forEachNode([this](const std::shared_ptr<EditorNode> &node) {
                std::shared_ptr<EditorNodeVoxelMesh> meshNode = std::dynamic_pointer_cast<EditorNodeVoxelMesh>(node);
                if (meshNode && meshNode->mesh) {
                    _api.resources->getOrLoadVoxelMesh(meshNode->resourcePath.data(), [meshNode, this](const std::vector<foundation::RenderDataPtr> &data, const util::Description& desc) {
                        meshNode->mesh = _api.scene->addVoxelMesh(data, desc);
                        meshNode->mesh->setPosition(meshNode->globalPosition);
                        _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
                    });
                }
            });
            
            _api.platform->sendEditorMsg("engine.resourcesReloaded", "");
            return true;
        }
        return false;
    }
    
    bool EditorVoxelMeshContext::_meshOffset(const std::string &data) {
        std::shared_ptr<EditorNodeVoxelMesh> node = std::dynamic_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock());
        if (node && node->mesh) {
            util::strstream input(data.c_str(), data.length());
            float x = 0, y = 0, z = 0;
            if (input >> x >> y >> z) {
                node->mesh->setCenterOffset(math::vector3f { x, y, z });
                _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
            }
        }
        
        return true;
    }

    bool EditorVoxelMeshContext::_save(const std::string &data) {
        std::shared_ptr<EditorNodeVoxelMesh> node = std::dynamic_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock());
        if (node && node->mesh) {
            const math::vector3f off = node->mesh->getCenterOffset();
            const std::string extPath = node->resourcePath + ".txt";
            
            if (std::fabs(off.x) + std::fabs(off.y) + std::fabs(off.z) > std::numeric_limits<float>::epsilon()) {
                util::Description desc;
                desc.setVector3f("offset", math::vector3f(off.x, off.y, off.z));
                util::Description *anims = desc.setDescription("animations");
                for (const auto &item : node->animations) {
                    anims->setVector3i(item.first.data(), {int(item.second.x), int(item.second.y), int(item.second.z)});
                }
                
                _savingCfg = util::Description::serialize(desc);
                _api.platform->saveFile(extPath.c_str(), reinterpret_cast<const std::uint8_t *>(_savingCfg.data()), _savingCfg.length(), [this, path = node->resourcePath](bool result) {
                    _api.platform->sendEditorMsg("engine.saved", path);
                });
            }
            else {
                // deleting
                _api.platform->saveFile(extPath.c_str(), nullptr, 0, [this, path = node->resourcePath](bool result) {
                    _api.platform->sendEditorMsg("engine.saved", path);
                });
            }
            return true;
        }
        return false;
    }

    void EditorVoxelMeshContext::_sendCurrentAnimParams(const EditorNodeVoxelMesh &node) {
        auto anim = node.animations.find(_currentAnimation);
        if (anim != node.animations.end()) {
            std::string params = _currentAnimation + " ";
            params += std::to_string(anim->second.x) + " " + std::to_string(anim->second.y) + " " + std::to_string(anim->second.z);
            _api.platform->sendEditorMsg("engine.mesh.animationUpdate", params);
        }
    }
    bool EditorVoxelMeshContext::_animationSelected(const std::string &data) {
        std::shared_ptr<EditorNodeVoxelMesh> node = std::dynamic_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock());
        if (node) {
            auto index = node->animations.find(data);
            if (index == node->animations.end()) {
                node->animations.emplace(data, math::vector3i { 0, 0, 100 });
            }
            
            _currentAnimation = data;
            _sendCurrentAnimParams(*node);
            
            if (_currentAnimationTime.has_value()) {
                _currentAnimationTime = 0.0f;
            }
        }
        
        return true;
    }
    bool EditorVoxelMeshContext::_animationRemoved(const std::string &data) {
        std::shared_ptr<EditorNodeVoxelMesh> node = std::dynamic_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock());
        if (node) {
            node->animations.erase(data);
            _currentAnimation = "";
            _currentAnimationTime.reset();
        }
        
        return true;
    }
    bool EditorVoxelMeshContext::_animationParameters(const std::string &data) {
        std::shared_ptr<EditorNodeVoxelMesh> node = std::dynamic_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock());
        if (node && node->mesh) {
            node->animations.erase(_currentAnimation);
            util::strstream input(data.c_str(), data.length());
            std::string name;
            std::uint32_t first, last, time;
            bool success = false;

            success |= input >> name;
            success |= input >> first >> last >> time;
            
            if (success) {
                first = std::min(first, node->mesh->getFrameCount() - 1);
                last = std::max(first, std::min(last, node->mesh->getFrameCount() - 1));
                node->animations.emplace(name, math::vector3i { int(first), int(last), int(time) });
                _currentAnimation = name;
                _sendCurrentAnimParams(*node);
            }
        }
        
        return true;
    }
    bool EditorVoxelMeshContext::_animationPlay(const std::string &data) {
        _currentAnimationTime = 0.0f;
        _animationLooped = data == "1";
        return true;
    }

}

