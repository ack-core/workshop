
#include "editor_static_mesh_context.h"

namespace game {
    EditorStaticMeshContext::EditorStaticMeshContext(API &&api, NodeAccessInterface &nodeAccess) : _api(std::move(api)), _nodeAccess(nodeAccess) {
        EditorNode::makeByType[std::size_t(EditorNodeType::STATIC)] = [](std::size_t typeIndex) {
            return std::static_pointer_cast<EditorNode>(std::make_shared<EditorNodeStaticMesh>(typeIndex));
        };
        
        _handlers["editor.selectNode"] = &EditorStaticMeshContext::_selectNode;
        _handlers["editor.static.set_mesh_path"] = &EditorStaticMeshContext::_setMeshPath;
        
        _editorEventsToken = _api.platform->addEditorEventHandler([this](const std::string &msg, const std::string &data) {
            auto handler = _handlers[msg];
            if (handler) {
                _api.platform->logMsg("[Static Mesh Context] msg = '%s' arg = '%s'", msg.data(), data.c_str());
                return (this->*handler)(data);
            }
            return false;
        });
    }
    
    EditorStaticMeshContext::~EditorStaticMeshContext() {
        _api.platform->removeEventHandler(_editorEventsToken);
    }
    
    void EditorStaticMeshContext::update(float dtSec) {
        if (std::shared_ptr<EditorNodeStaticMesh> node = std::dynamic_pointer_cast<EditorNodeStaticMesh>(_nodeAccess.getSelectedNode().lock())) {
            if (node->mesh) {
                node->mesh->setPosition(node->position);
            }
        }
    }
    
    bool EditorStaticMeshContext::_selectNode(const std::string &data) {
        if (std::shared_ptr<EditorNodeStaticMesh> node = std::dynamic_pointer_cast<EditorNodeStaticMesh>(_nodeAccess.getSelectedNode().lock())) {
            _api.platform->sendEditorMsg("engine.nodeSelected", data + " inspect_static_mesh " + node->meshPath);
            return true;
        }
        return false;
    }
    
    bool EditorStaticMeshContext::_setMeshPath(const std::string &data) {
        std::shared_ptr<EditorNodeStaticMesh> node = std::static_pointer_cast<EditorNodeStaticMesh>(_nodeAccess.getSelectedNode().lock());
        if (node) {
            node->meshPath = data;
            
            _api.resources->getOrLoadVoxelStatic(data.c_str(), [node, weak = weak_from_this()](const foundation::RenderDataPtr &data) {
                if (std::shared_ptr<EditorStaticMeshContext> self = weak.lock()) {
                    node->mesh = self->_api.scene->addStaticMesh(data);
                    self->_api.platform->sendEditorMsg("engine.refresh", "");
                }
            });
        }
        return true;
    }
}

