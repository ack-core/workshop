
#include "editor_collision_shape_context.h"

namespace game {
    void EditorNodeCollisionShape::update(float dtSec) {

    }
    void EditorNodeCollisionShape::setResourcePath(const API &api, const std::string &path) {
        if (path.empty()) {
            resourcePath = EDITOR_EMPTY_RESOURCE_PATH;
        }
        else {
            resourcePath = path;
        }
    }

    EditorCollisionShapeContext::EditorCollisionShapeContext(API &&api, NodeAccessInterface &nodeAccess, CameraAccessInterface &cameraAccess)
    : _api(std::move(api))
    , _nodeAccess(nodeAccess)
    , _cameraAccess(cameraAccess)
    {
        EditorNode::makeByType[std::size_t(voxel::WorldInterface::NodeType::COLLISION)] = [](std::size_t typeIndex) {
            return std::static_pointer_cast<EditorNode>(std::make_shared<EditorNodeCollisionShape>(typeIndex));
        };
        
        _handlers["editor.selectNode"] = &EditorCollisionShapeContext::_selectNode;
        _handlers["editor.setPath"] = &EditorCollisionShapeContext::_setResourcePath;
        _handlers["editor.startEditing"] = &EditorCollisionShapeContext::_startEditing;
        _handlers["editor.stopEditing"] = &EditorCollisionShapeContext::_stopEditing;
        _handlers["editor.reloadResources"] = &EditorCollisionShapeContext::_reload;
        _handlers["editor.resource.save"] = &EditorCollisionShapeContext::_save;

        _editorEventsToken = _api.platform->addEditorEventHandler([this](const std::string &msg, const std::string &data) {
            auto handler = _handlers[msg];
            if (handler) {
                return (this->*handler)(data);
            }
            return false;
        });
    }
    
    EditorCollisionShapeContext::~EditorCollisionShapeContext() {
        _api.platform->removeEventHandler(_editorEventsToken);
    }
    
    void EditorCollisionShapeContext::update(float dtSec) {

    }
    
    bool EditorCollisionShapeContext::_selectNode(const std::string &data) {
        if (std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock())) {
            _api.platform->sendEditorMsg("engine.nodeSelected", data + " inspect_collision " + node->resourcePath);
        }
        return false;
    }
    
    bool EditorCollisionShapeContext::_setResourcePath(const std::string &data) {
        if (std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock())) {
            node->setResourcePath(_api, data);
            return true;
        }
        return false;
    }
        
    bool EditorCollisionShapeContext::_startEditing(const std::string &data) {
        std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock());
        if (node) {
            _api.platform->sendEditorMsg("engine.collision.editing", "");
            return true;
        }
        
        return false;
    }
    
    bool EditorCollisionShapeContext::_stopEditing(const std::string &data) {
        std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock());
        if (node) {

            return true;
        }
        return false;
    }
        
    bool EditorCollisionShapeContext::_reload(const std::string &data) {
        std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock());
        if (node) {

            return true;
        }
        return false;
    }

    bool EditorCollisionShapeContext::_save(const std::string &data) {
        std::shared_ptr<EditorNodeCollisionShape> node = std::dynamic_pointer_cast<EditorNodeCollisionShape>(_nodeAccess.getSelectedNode().lock());
        if (node) {

            return true;
        }
        return false;
    }

}

