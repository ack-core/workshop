
#include "editor_prefab_context.h"

namespace game {
    void EditorNodePrefab::update(float dtSec) {

    }

    EditorPrefabContext::EditorPrefabContext(API &&api, NodeAccessInterface &nodeAccess) : _api(std::move(api)), _nodeAccess(nodeAccess) {
        EditorNode::makeByType[std::size_t(EditorNodeType::PREFAB)] = [](std::size_t typeIndex) {
            return std::static_pointer_cast<EditorNode>(std::make_shared<EditorNodePrefab>(typeIndex));
        };
        
        _handlers["editor.selectNode"] = &EditorPrefabContext::_selectNode;
        _handlers["editor.setPath"] = &EditorPrefabContext::_setResourcePath;
        _handlers["editor.startEditing"] = &EditorPrefabContext::_startEditing;

        _editorEventsToken = _api.platform->addEditorEventHandler([this](const std::string &msg, const std::string &data) {
            auto handler = _handlers[msg];
            if (handler) {
                _api.platform->logMsg("[Prefab Context] msg = '%s' arg = '%s'", msg.data(), data.c_str());
                return (this->*handler)(data);
            }
            return false;
        });
    }
    
    EditorPrefabContext::~EditorPrefabContext() {
        _api.platform->removeEventHandler(_editorEventsToken);
    }
    
    void EditorPrefabContext::update(float dtSec) {

    }
    
    bool EditorPrefabContext::_selectNode(const std::string &data) {
        if (std::shared_ptr<EditorNodePrefab> node = std::dynamic_pointer_cast<EditorNodePrefab>(_nodeAccess.getSelectedNode().lock())) {
            _api.platform->sendEditorMsg("engine.nodeSelected", data + " inspect_prefab " + node->prefabPath);
        }
        return false;
    }
    
    bool EditorPrefabContext::_setResourcePath(const std::string &data) {
        if (std::shared_ptr<EditorNodePrefab> node = std::dynamic_pointer_cast<EditorNodePrefab>(_nodeAccess.getSelectedNode().lock())) {
            node->prefabPath = data;
            
//            _api.resources->getOrLoadVoxelMesh(data.c_str(), [node, this](const std::vector<foundation::RenderDataPtr> &data, const util::IntegerOffset3D& offset) {
//                node->mesh = _api.scene->addVoxelMesh(data, offset);
//                _api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
//            });
            
            return true;
        }
        return false;
    }
        
    bool EditorPrefabContext::_startEditing(const std::string &data) {

        return false;
    }
    

}

