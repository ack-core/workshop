
#include "editor_ground_context.h"
#include "utils.h"

namespace game {
    void EditorNodeGround::update(float dtSec) {
        globalPosition = localPosition + (parent ? parent->globalPosition : math::vector3f(0, 0, 0));
        if (mesh) {
            mesh->setPosition(globalPosition);
        }
    }
    void EditorNodeGround::setResourcePath(const API &api, const std::string &path) {
        if (path.empty()) {
            resourcePath = EDITOR_EMPTY_RESOURCE_PATH;
            mesh = nullptr;
        }
        else {
            resourcePath = path;
            api.resources->getOrLoadGround(path.c_str(), [this, &api](const foundation::RenderDataPtr &m, const foundation::RenderTexturePtr &t) {
                mesh = api.scene->addTexturedMesh(m, t);
                api.platform->sendEditorMsg("engine.refresh", EDITOR_REFRESH_PARAM);
            });
        }
    }

    EditorGroundContext::EditorGroundContext(API &&api, NodeAccessInterface &nodeAccess) : _api(std::move(api)), _nodeAccess(nodeAccess) {
        EditorNode::makeByType[std::size_t(core::WorldInterface::NodeType::GROUND)] = [](std::size_t typeIndex) {
            return std::static_pointer_cast<EditorNode>(std::make_shared<EditorNodeGround>(typeIndex));
        };
        
        _handlers["editor.selectNode"] = &EditorGroundContext::_selectNode;
        _handlers["editor.setPath"] = &EditorGroundContext::_setResourcePath;
        _handlers["editor.startEditing"] = &EditorGroundContext::_startEditing;
        _handlers["editor.stopEditing"] = &EditorGroundContext::_stopEditing;
        _handlers["editor.reloadResources"] = &EditorGroundContext::_reload;
        _handlers["editor.resource.save"] = &EditorGroundContext::_save;
        
        _editorEventsToken = _api.platform->addEditorEventHandler([this](const std::string &msg, const std::string &data) {
            auto handler = _handlers[msg];
            if (handler) {
                return (this->*handler)(data);
            }
            return false;
        });
    }
    
    EditorGroundContext::~EditorGroundContext() {
        _api.platform->removeEventHandler(_editorEventsToken);
    }
    
    void EditorGroundContext::update(float dtSec) {

    }
    
    bool EditorGroundContext::_selectNode(const std::string &data) {
        if (std::shared_ptr<EditorNodeGround> node = std::dynamic_pointer_cast<EditorNodeGround>(_nodeAccess.getSelectedNode().lock())) {
            _api.platform->sendEditorMsg("engine.nodeSelected", data + " inspect_ground " + node->resourcePath);
        }
        return false;
    }
    
    bool EditorGroundContext::_setResourcePath(const std::string &data) {
        if (std::shared_ptr<EditorNodeGround> node = std::dynamic_pointer_cast<EditorNodeGround>(_nodeAccess.getSelectedNode().lock())) {
            node->setResourcePath(_api, data);
            return true;
        }
        return false;
    }
        
    bool EditorGroundContext::_startEditing(const std::string &data) {
        std::shared_ptr<EditorNodeGround> node = std::dynamic_pointer_cast<EditorNodeGround>(_nodeAccess.getSelectedNode().lock());
        if (node && node->mesh) {
            _api.platform->sendEditorMsg("engine.ground.editing", "");

            
            return true;
        }
        
        return false;
    }
    
    bool EditorGroundContext::_stopEditing(const std::string &data) {
        std::shared_ptr<EditorNodeGround> node = std::dynamic_pointer_cast<EditorNodeGround>(_nodeAccess.getSelectedNode().lock());
        if (node && node->mesh) {

            return true;
        }
        return false;
    }
        
    bool EditorGroundContext::_reload(const std::string &data) {
        std::shared_ptr<EditorNodeGround> node = std::dynamic_pointer_cast<EditorNodeGround>(_nodeAccess.getSelectedNode().lock());
        if (node && node->mesh) {

            _api.platform->sendEditorMsg("engine.resourcesReloaded", "");
            return true;
        }
        return false;
    }

    bool EditorGroundContext::_save(const std::string &data) {
        std::shared_ptr<EditorNodeGround> node = std::dynamic_pointer_cast<EditorNodeGround>(_nodeAccess.getSelectedNode().lock());
        if (node && node->mesh) {

            return true;
        }
        return false;
    }


}

