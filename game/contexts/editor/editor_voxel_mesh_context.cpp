
#include "editor_voxel_mesh_context.h"

namespace game {
    EditorVoxelMeshContext::EditorVoxelMeshContext(API &&api, NodeAccessInterface &nodeAccess) : _api(std::move(api)), _nodeAccess(nodeAccess) {
        EditorNode::makeByType[std::size_t(EditorNodeType::MESH)] = [](std::size_t typeIndex) {
            return std::static_pointer_cast<EditorNode>(std::make_shared<EditorNodeVoxelMesh>(typeIndex));
        };
        
        _handlers["editor.selectNode"] = &EditorVoxelMeshContext::_selectNode;
        _handlers["editor.mesh.setPath"] = &EditorVoxelMeshContext::_setMeshPath;
        _handlers["editor.startEditing"] = &EditorVoxelMeshContext::_startEditing;
        _handlers["editor.stopEditing"] = &EditorVoxelMeshContext::_stopEditing;
        _handlers["editor.mesh.offset"] = &EditorVoxelMeshContext::_meshOffset;
        _handlers["editor.mesh.save"] = &EditorVoxelMeshContext::_save;
        _handlers["editor.reload"] = &EditorVoxelMeshContext::_reload;
        
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
        if (std::shared_ptr<EditorNodeVoxelMesh> node = std::dynamic_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock())) {
            if (node->mesh) {
                node->mesh->setPosition(node->position);
            }
        }
    }
    
    bool EditorVoxelMeshContext::_selectNode(const std::string &data) {
        if (std::shared_ptr<EditorNodeVoxelMesh> node = std::dynamic_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock())) {
            _api.platform->sendEditorMsg("engine.nodeSelected", data + " inspect_mesh " + node->meshPath);
        }
        return false;
    }
    
    bool EditorVoxelMeshContext::_setMeshPath(const std::string &data) {
        std::shared_ptr<EditorNodeVoxelMesh> node = std::static_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock());
        if (node) {
            node->meshPath = data;
            
            _api.resources->getOrLoadVoxelMesh(data.c_str(), [node, this](const std::vector<foundation::RenderDataPtr> &data, const util::IntegerOffset3D& offset) {
                node->mesh = _api.scene->addVoxelMesh(data, offset);
                _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
            });
        }
        return true;
    }
    
    bool EditorVoxelMeshContext::_startEditing(const std::string &data) {
        std::shared_ptr<EditorNodeVoxelMesh> node = std::static_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock());
        if (node && node->mesh) {
            util::IntegerOffset3D offset = node->mesh->getCenterOffset();
            _api.platform->sendEditorMsg("engine.mesh.editing", std::to_string(offset.x) + " " + std::to_string(offset.y) + " " + std::to_string(offset.z));
            return true;
        }
        
        return false;
    }

    bool EditorVoxelMeshContext::_meshOffset(const std::string &data) {
        std::shared_ptr<EditorNodeVoxelMesh> node = std::static_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock());
        if (node && node->mesh) {
            util::strstream input(data.c_str(), data.length());
            int x = 0, y = 0, z = 0;
            if (input >> x >> y >> z) {
                const util::IntegerOffset3D offset = node->mesh->getCenterOffset();
                node->mesh->setCenterOffset(util::IntegerOffset3D { offset.x + x, offset.y + y, offset.z + z });
                _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
            }
        }
        
        return true;
    }
    
    bool EditorVoxelMeshContext::_stopEditing(const std::string &data) {
        std::shared_ptr<EditorNodeVoxelMesh> node = std::static_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock());
        if (node && node->mesh) {
            node->mesh->resetOffset();
            _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
            return true;
        }
        return false;
    }
    
    bool EditorVoxelMeshContext::_save(const std::string &data) {
        std::shared_ptr<EditorNodeVoxelMesh> node = std::static_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock());
        if (node && node->mesh) {
            util::IntegerOffset3D off = node->mesh->getCenterOffset();
            
            if (off.x || off.y || off.z) {
                std::string cfg = "parameters {\r\n    offset : vector3f = " + std::to_string(off.x) + " " + std::to_string(off.y) + " " + std::to_string(off.z) + "\r\n}\r\n";
                
                _api.platform->saveFile((node->meshPath + ".txt").c_str(), reinterpret_cast<const std::uint8_t *>(cfg.data()), cfg.length(), [this](bool result) {
                    _api.platform->sendEditorMsg("engine.saved", std::to_string(int(result)));
                });
            }
            else {
                // deleting
                _api.platform->saveFile((node->meshPath + ".txt").c_str(), nullptr, 0, [this](bool result) {
                    _api.platform->sendEditorMsg("engine.saved", std::to_string(int(result)));
                });
            }

        }
        return true;
    }
    
    bool EditorVoxelMeshContext::_reload(const std::string &data) {
        std::shared_ptr<EditorNodeVoxelMesh> node = std::static_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock());
        if (node && node->mesh) {
            _api.resources->removeMesh(node->meshPath.data());
            
            // updating all mesh nodes
            _nodeAccess.forEachNode([this](const std::shared_ptr<EditorNode> &node) {
                std::shared_ptr<EditorNodeVoxelMesh> meshNode = std::dynamic_pointer_cast<EditorNodeVoxelMesh>(node);
                if (meshNode && meshNode->mesh) {
                    _api.resources->getOrLoadVoxelMesh(meshNode->meshPath.data(), [meshNode, this](const std::vector<foundation::RenderDataPtr> &data, const util::IntegerOffset3D& offset) {
                        meshNode->mesh = _api.scene->addVoxelMesh(data, offset);
                        meshNode->mesh->setPosition(meshNode->position);
                        _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
                    });
                }
            });
            return true;
        }
        return false;
    }

}

