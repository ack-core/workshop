
#include "editor_raycast_shape_context.h"

namespace game {
    void EditorNodeRaycastShape::update(float dtSec) {

    }
    void EditorNodeRaycastShape::setResourcePath(const API &api, const std::string &path) {
        if (path.empty()) {
            resourcePath = EDITOR_EMPTY_RESOURCE_PATH;
        }
        else {
            resourcePath = path;
        }
    }

    EditorRaycastShapeContext::EditorRaycastShapeContext(API &&api, NodeAccessInterface &nodeAccess, CameraAccessInterface &cameraAccess)
    : _api(std::move(api))
    , _nodeAccess(nodeAccess)
    , _cameraAccess(cameraAccess)
    {
        EditorNode::makeByType[std::size_t(voxel::WorldInterface::NodeType::RAYCAST)] = [](std::size_t typeIndex) {
            return std::static_pointer_cast<EditorNode>(std::make_shared<EditorNodeRaycastShape>(typeIndex));
        };
        
        _handlers["editor.selectNode"] = &EditorRaycastShapeContext::_selectNode;
        _handlers["editor.setPath"] = &EditorRaycastShapeContext::_setResourcePath;
        _handlers["editor.startEditing"] = &EditorRaycastShapeContext::_startEditing;
        _handlers["editor.stopEditing"] = &EditorRaycastShapeContext::_stopEditing;
        _handlers["editor.reloadResources"] = &EditorRaycastShapeContext::_reload;
        _handlers["editor.resource.save"] = &EditorRaycastShapeContext::_save;

        _editorEventsToken = _api.platform->addEditorEventHandler([this](const std::string &msg, const std::string &data) {
            auto handler = _handlers[msg];
            if (handler) {
                return (this->*handler)(data);
            }
            return false;
        });
    }
    
    EditorRaycastShapeContext::~EditorRaycastShapeContext() {
        _api.platform->removeEventHandler(_editorEventsToken);
    }
    
    void EditorRaycastShapeContext::update(float dtSec) {

    }
    
    bool EditorRaycastShapeContext::_selectNode(const std::string &data) {
        if (std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock())) {
            _api.platform->sendEditorMsg("engine.nodeSelected", data + " inspect_raycast " + node->resourcePath);
        }
        return false;
    }
    
    bool EditorRaycastShapeContext::_setResourcePath(const std::string &data) {
        if (std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock())) {
            node->setResourcePath(_api, data);
            return true;
        }
        return false;
    }
        
    bool EditorRaycastShapeContext::_startEditing(const std::string &data) {
        std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock());
        if (node) {
            _api.platform->sendEditorMsg("engine.raycast.editing", "");
            return true;
        }
        
        return false;
    }
    
    bool EditorRaycastShapeContext::_stopEditing(const std::string &data) {
        std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock());
        if (node) {

            return true;
        }
        return false;
    }
        
    bool EditorRaycastShapeContext::_reload(const std::string &data) {
        std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock());
        if (node) {

            return true;
        }
        return false;
    }

    bool EditorRaycastShapeContext::_save(const std::string &data) {
        std::shared_ptr<EditorNodeRaycastShape> node = std::dynamic_pointer_cast<EditorNodeRaycastShape>(_nodeAccess.getSelectedNode().lock());
        if (node) {

            return true;
        }
        return false;
    }

}

