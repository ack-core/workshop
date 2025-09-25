
#include "editor_voxel_mesh_context.h"

namespace game {
    EditorVoxelMeshContext::EditorVoxelMeshContext(API &&api, NodeAccessInterface &nodeAccess) : _api(std::move(api)), _nodeAccess(nodeAccess) {
        EditorNode::makeByType[std::size_t(EditorNodeType::MESH)] = [](std::size_t typeIndex) {
            return std::static_pointer_cast<EditorNode>(std::make_shared<EditorNodeVoxelMesh>(typeIndex));
        };
        
        _handlers["editor.selectNode"] = &EditorVoxelMeshContext::_selectNode;
        _handlers["editor.mesh.set_path"] = &EditorVoxelMeshContext::_setMeshPath;
        
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
            
//            _api.platform->saveFile("test/aaa/cc.txt", (const std::uint8_t *)"hello!1", 7, [this](bool result) {
//                if (result) {
//                    _api.platform->logMsg("file saved!");
//                }
//                else {
//                    _api.platform->logMsg("file saving error!");
//                }
//            });
        }
        return false;
    }
    
    bool EditorVoxelMeshContext::_setMeshPath(const std::string &data) {
        std::shared_ptr<EditorNodeVoxelMesh> node = std::static_pointer_cast<EditorNodeVoxelMesh>(_nodeAccess.getSelectedNode().lock());
        if (node) {
            node->meshPath = data;
            
            _api.resources->getOrLoadVoxelMesh(data.c_str(), [node, weak = weak_from_this()](const std::vector<foundation::RenderDataPtr> &data) {
                if (std::shared_ptr<EditorVoxelMeshContext> self = weak.lock()) {
                    node->mesh = self->_api.scene->addVoxelMesh(data);
                    self->_api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
                }
            });
        }
        return true;
    }
}

