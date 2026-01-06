
#include "editor_prefab_context.h"

namespace game {
    void EditorNodePrefab::update(float dtSec) {

    }

    void EditorNodePrefab::setResourcePath(const API &api, const std::string &path) {
        if (path.empty()) {
            resourcePath = EDITOR_EMPTY_RESOURCE_PATH;
            prefabDesc = {};
        }
        else {
            resourcePath = path;
            prefabDesc = api.resources->getPrefab(path.c_str());
        }
    }

    EditorPrefabContext::EditorPrefabContext(API &&api, NodeAccessInterface &nodeAccess) : _api(std::move(api)), _nodeAccess(nodeAccess) {
        EditorNode::makeByType[std::size_t(EditorNodeType::PREFAB)] = [](std::size_t typeIndex) {
            return std::static_pointer_cast<EditorNode>(std::make_shared<EditorNodePrefab>(typeIndex));
        };
        
        _handlers["editor.selectNode"] = &EditorPrefabContext::_selectNode;
        _handlers["editor.setPath"] = &EditorPrefabContext::_setResourcePath;
        _handlers["editor.startEditing"] = &EditorPrefabContext::_startEditing;
        _handlers["editor.savePrefab"] = &EditorPrefabContext::_savePrefab;
        _handlers["editor.reloadPrefabs"] = &EditorPrefabContext::_reloadPrefabs;

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
            _api.platform->sendEditorMsg("engine.nodeSelected", data + " inspect_prefab " + node->resourcePath);
        }
        return false;
    }
    
    bool EditorPrefabContext::_setResourcePath(const std::string &data) {
        if (std::shared_ptr<EditorNodePrefab> node = std::dynamic_pointer_cast<EditorNodePrefab>(_nodeAccess.getSelectedNode().lock())) {
            node->setResourcePath(_api, data);
            return true;
        }
        return false;
    }
    
    bool EditorPrefabContext::_startEditing(const std::string &data) {
        if (std::shared_ptr<EditorNodePrefab> node = std::dynamic_pointer_cast<EditorNodePrefab>(_nodeAccess.getSelectedNode().lock())) {
            if (node->prefabDesc.empty() == false) {
                for (auto &index : node->prefabDesc) {
                    const util::Description *desc = std::any_cast<util::Description>(&index.second);
                    if (desc) {
                        math::vector3f position = desc->get<math::vector3f>("position");
                        
                        if (index.first.find('.') == std::string::npos) { // root node comes first
                            if (_nodeAccess.hasNodeWithName(index.first)) {
                                _api.platform->sendEditorMsg("engine.prefab.editFail", index.first);
                                break;
                            }
                            position = node->localPosition; // place tree where the prefab is
                        }

                        const EditorNodeType type = desc->get<EditorNodeType>("type");
                        const std::string resourcePath = desc->get<std::string>("resourcePath");
                        _nodeAccess.createNode(type, index.first, position, resourcePath);
                        _api.platform->sendEditorMsg("engine.insertNode", index.first);
                    }
                }
                _api.platform->sendEditorMsg("engine.prefab.editing", "");
            }
            return true;
        }
        return false;
    }

    bool EditorPrefabContext::_validatePrefab(const EditorNode &rootNode) {
        struct fn {
            static bool validateNode(const EditorNode &node) {
                bool error = false;
                
                error |= node.resourcePath == EDITOR_EMPTY_RESOURCE_PATH;
                
                for (auto &item : node.children) {
                    error |= validateNode(*item.second);
                }
                
                return error;
            }
        };
        return fn::validateNode(rootNode);
    }
    
    bool EditorPrefabContext::_savePrefab(const std::string &data) {
        struct fn {
            static void addNode(util::Description &out, EditorNode &node, bool isRoot) {
                const math::vector3f position = isRoot ? math::vector3f(0, 0, 0) : node.localPosition;
                util::Description &nodeDesc = out.addSubDesc(node.name);
                nodeDesc.set("type", int(node.type));
                nodeDesc.set("position", position);
                nodeDesc.set("resourcePath", node.resourcePath);
                
                for (auto &item : node.children) {
                    addNode(out, *item.second, false);
                }
            }
        };
        if (std::shared_ptr<EditorNode> node = _nodeAccess.getSelectedNode().lock()) {
            std::string errorString;
            
            // ignore without messagebox
            if (node->type == EditorNodeType::PREFAB || node->parent != nullptr) {
                return true;
            }
            
            if (_validatePrefab(*node) == false) { // no errors
                util::Description prefabDesc;
                fn::addNode(prefabDesc, *node, true);
                _api.platform->sendEditorMsg("engine.savePrefab", util::serializeDescription(prefabDesc));
                return true;
            }
            else {
                errorString = "Subtree is not valid for saving";
            }
            
            _api.platform->sendEditorMsg("engine.prefab.saveFail", errorString);
        }
        
        return true;
    }

    bool EditorPrefabContext::_reloadPrefabs(const std::string &data) {
        _api.resources->reloadPrefabs([this]() {
            _api.platform->sendEditorMsg("engine.resourcesReloaded", "");
        });
        return true;
    }
}

