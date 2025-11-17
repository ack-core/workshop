
#include "editor_voxel_mesh_context.h"

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
            api.resources->getOrLoadVoxelMesh(path.c_str(), [this, &api](const std::vector<foundation::RenderDataPtr> &data, const util::IntegerOffset3D& offset) {
                mesh = api.scene->addVoxelMesh(data, offset);
                api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
            });
        }
    }

    EditorVoxelMeshContext::EditorVoxelMeshContext(API &&api, NodeAccessInterface &nodeAccess) : _api(std::move(api)), _nodeAccess(nodeAccess) {
        EditorNode::makeByType[std::size_t(EditorNodeType::MESH)] = [](std::size_t typeIndex) {
            return std::static_pointer_cast<EditorNode>(std::make_shared<EditorNodeVoxelMesh>(typeIndex));
        };
        
        _handlers["editor.selectNode"] = &EditorVoxelMeshContext::_selectNode;
        _handlers["editor.setPath"] = &EditorVoxelMeshContext::_setResourcePath;
        _handlers["editor.startEditing"] = &EditorVoxelMeshContext::_startEditing;
        _handlers["editor.stopEditing"] = &EditorVoxelMeshContext::_stopEditing;
        _handlers["editor.reloadResources"] = &EditorVoxelMeshContext::_reload;
        _handlers["editor.mesh.offset"] = &EditorVoxelMeshContext::_meshOffset;
        _handlers["editor.resource.save"] = &EditorVoxelMeshContext::_save;

        _editorEventsToken = _api.platform->addEditorEventHandler([this](const std::string &msg, const std::string &data) {
            auto handler = _handlers[msg];
            if (handler) {
                _api.platform->logMsg("[Voxel Mesh Context] msg = '%s' arg = '%s'", msg.data(), data.c_str());
                return (this->*handler)(data);
            }
            return false;
        });
    }
    
    EditorVoxelMeshContext::~EditorVoxelMeshContext() {
        _api.platform->removeEventHandler(_editorEventsToken);
    }
    
    void EditorVoxelMeshContext::update(float dtSec) {

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
            util::IntegerOffset3D offset = node->mesh->getCenterOffset();
            _api.platform->sendEditorMsg("engine.mesh.editing", std::to_string(offset.x) + " " + std::to_string(offset.y) + " " + std::to_string(offset.z));
            return true;
        }
        
        return false;
    }
    
    bool EditorVoxelMeshContext::_stopEditing(const std::string &data) {
        std::shared_ptr<EditorNodeVoxelMesh> node = std::dynamic_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock());
        if (node && node->mesh) {
            node->mesh->resetOffset();
            _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
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
                    _api.resources->getOrLoadVoxelMesh(meshNode->resourcePath.data(), [meshNode, this](const std::vector<foundation::RenderDataPtr> &data, const util::IntegerOffset3D& offset) {
                        meshNode->mesh = _api.scene->addVoxelMesh(data, offset);
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
            int x = 0, y = 0, z = 0;
            if (input >> x >> y >> z) {
                node->mesh->setCenterOffset(util::IntegerOffset3D { x, y, z });
                _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
            }
        }
        
        return true;
    }

    bool EditorVoxelMeshContext::_save(const std::string &data) {
        std::shared_ptr<EditorNodeVoxelMesh> node = std::dynamic_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock());
        if (node && node->mesh) {
            util::IntegerOffset3D off = node->mesh->getCenterOffset();
            const std::string extPath = node->resourcePath + ".txt";
            
            if (off.x || off.y || off.z) {
                util::Description desc;
                util::Description &parameters = desc.addSubDesc("parameters");
                parameters.set("offset", math::vector3f(off.x, off.y, off.z));
                _savingCfg = util::serializeDescription(desc);
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

}

